
#include "Search.h"

void Shift_iters(borders &bord_Z, borders &bord_X, std::list<double>::iterator &iter_x, std::list<double>::iterator &iter_z)
{
	bord_Z.left = *iter_z++;
	bord_Z.right = *iter_z;
	bord_X.left = *iter_x++;
	bord_X.right = *iter_x;
}
PointerOneDim Search::Simple_Par_searchMin(const double _left, const double _right, const int _N_max, const double _Eps, const int _threads)
{
	procs = _threads;

	std::vector<PointerOneDim> res_threads(procs); // вектор для хранения результатов каждого потока
	omp_set_dynamic(0);

	double start = omp_get_wtime();
	omp_set_num_threads(procs);


	if (_threads > 1)
	{

#pragma omp parallel for
		for (int i = 0; i < procs; i++)//выполняем поиск на отдельных участках отрезка
		{
			res_threads[i] = Serial_searchMin(_left + (_right - _left)*i / procs, _left + (_right - _left)*(i + 1) / procs, _N_max, _Eps);

		}

		PointerOneDim res;
		res.z = res_threads[0].z;
		res.x = res_threads[0].x;
		res.steps = 0;
		res.time = 0;
		for (int i = 0; i < procs; i++)
		{
			if (res_threads[i].z < res.z)//находим меньший среди найденных
			{
				res.z = res_threads[i].z;
				res.x = res_threads[i].x;

			}
			res.time += res_threads[i].time;//суммируем общее время работы каждого потока
			res.steps += res_threads[i].steps;//суммируем общее число итераций кажлого потока

			for (int j = 0; j < res_threads[i].X.size(); j++)//сохраняем полученные координаты в один общий вектор
			{
				res.X.push_back(res_threads[i].X[j]);
			}

		}
		return res;
	}

	else//один поток
	{

		res_threads[0] = Serial_searchMin(_left, _right, _N_max, _Eps);
		PointerOneDim res;
		res.z = res_threads[0].z;
		res.x = res_threads[0].x;
		res.time = 0;
		res.steps = res_threads[0].steps;
		for (int j = 0; j < res_threads[0].X.size(); j++)
		{

			res.X.push_back(res_threads[0].X[j]);
		}
		res.time += res_threads[0].time;
		return res;
	}


}



PointerOneDim Search::Serial_searchMin(const double _left, const double _right, const int _N_max, const double _Eps)
{
	PointerOneDim p;
	p.steps = 1;
	p.time = 0;


	double curr_left = _left;

	double curr_right = _right;

	double start, end;


	double z_begin = Func(curr_left); // левая граница
	double z_end = Func(curr_right); // правая граница

	double temp = 0.; // переменная для подсчёта 
	int localSteps = 1; // количество шагов в Настоящем алгоритме
	int all_steps;
	double M_big;
	double m_small = 1;
	int R_max_index = 0; // Переменная индекса следующей точки испытания

	double new_X; // переменные для рабочего отрезка и точки нового испытания

	std::list<double> M_vector;
	std::list<double> R_vector;

	std::list<double> z; //значения Z
	std::list<double> x; //значения рабочих X

	borders bord_Z; // границы для Z
	borders bord_X; // границы для X

	//деление на интервалы работает так:
	//1) вставил новую точку x в текущий отрезок
	//2) новым интервалом считается получившаяся левая половина
	//3) правая половина считаю старой

	std::list<double>::iterator place_M, place_R; // указатели для места установки нового значения
	std::list<double>::iterator iter_x, iter_z, iter_R, iter_M;

	M_big = M(z_end, z_begin, curr_right, curr_left);
	Lipschitz(M_big, m_small);
	new_X = New_x(curr_right, curr_left, z_end, z_begin, m_small);


	x.push_back(curr_left);
	x.push_back(new_X);
	x.push_back(curr_right);

	iter_x = x.begin();
	for (int i = 0; i < 3; i++)
	{
		z.push_back(Func(*iter_x++));
	}

	iter_z = z.begin();
	iter_x = x.begin();




	start = omp_get_wtime();
	p.X.push_back(curr_right);
	//первая итерация
	for (int i = 1; i < 3; i++)
	{
		Shift_iters(bord_Z, bord_X, iter_x, iter_z);

		M_vector.push_back(M(bord_Z.right, bord_Z.left, bord_X.right, bord_X.left)); // вставка новой М_big
	}

	double max = M_vector.front();

	if (max < M_vector.back()) max = M_vector.back();
	M_big = max; // поиск максимальной M_big

	Lipschitz(M_big, m_small);

	R_max_index = 1;

	iter_z = z.begin();
	iter_x = x.begin();
	double tmp;
	for (int i = 1; i < 3; i++)
	{
		Shift_iters(bord_Z, bord_X, iter_x, iter_z);

		tmp = R(m_small, bord_Z.right, bord_Z.left, bord_X.right, bord_X.left);
		R_vector.push_back(tmp);
		if (i == 1) max = tmp;

		if (max < tmp)
		{
			max = tmp;
			R_max_index = i;
		}
	}

	iter_z = z.begin();
	iter_x = x.begin();

	for (int i = 0; i < R_max_index; i++)
	{
		iter_z++;
		iter_x++;
	}

	curr_right = *iter_x;
	curr_left = *--iter_x;
	p.X.push_back(curr_right);

	//основной цикл
	while (true)
	{
		all_steps = 2 + localSteps;
		iter_z = z.begin(); // найдём интервалы с максимальной характеристикой
		iter_x = x.begin();
		// ищем интервалы сдвигая итераторы
		for (register int i = 0; i < R_max_index; i++)
		{
			iter_z++;
			iter_x++;
		}
		// забираем значения для хранения и работы нового цикла
		bord_Z.right = *iter_z--;
		bord_Z.left = *iter_z++;

		// вычисление новой точки испытания x_new
		new_X = New_x(curr_right, curr_left, bord_Z.right, bord_Z.left, m_small);

		// сохраняем x_new и z_new
		z.insert(iter_z, Func(new_X));
		x.insert(iter_x, new_X);

		iter_z = z.begin();
		iter_x = x.begin();
		place_M = M_vector.begin();
		place_R = R_vector.begin();

		// ищем интервал с лучшей R, двигая iter
		for (int i = 0; i < R_max_index - 1; i++)
		{
			iter_z++;
			iter_x++;
			if (i == R_max_index - 1)
			{
				break;
			}
			place_M++;
			place_R++;
		}

		for (int i = 0; i < 2; i++)
		{
			Shift_iters(bord_Z, bord_X, iter_x, iter_z);
			if (i == 0)
			{
				M_vector.insert(place_M, M(bord_Z.right, bord_Z.left, bord_X.right, bord_X.left));//В новый интервал
			}
			else
			{
				*place_M = M(bord_Z.right, bord_Z.left, bord_X.right, bord_X.left); // старый интервал
			}
		}

		iter_M = M_vector.begin();
		double temp;
		double max = *iter_M;
		temp = M_vector.back();

		for (int i = 0; i < all_steps - 1; i++)
		{
			if (max < temp) max = temp;
			temp = *++iter_M;
		}
		M_big = max;

		Lipschitz(M_big, m_small);



		iter_z = z.begin();
		iter_x = x.begin();

		R_max_index = 1;
		place_R = R_vector.begin();

		for (int i = 0; i < all_steps; i++)
		{
			Shift_iters(bord_Z, bord_X, iter_x, iter_z);
			if (i == 0)
			{
				R_vector.insert(place_R, R(m_small, bord_Z.right, bord_Z.left, bord_X.right, bord_X.left));//В новый интервал
			}
			else
			{
				*place_R++ = R(m_small, bord_Z.right, bord_Z.left, bord_X.right, bord_X.left); // старый интервал
			}
		}

		iter_R = R_vector.begin();
		max = *iter_R;
		for (int i = 1; i < all_steps; i++)
		{
			temp = *++iter_R;
			if (max < temp)
			{
				max = temp;
				R_max_index = i + 1;
			}
		}

		iter_x = x.begin();
		iter_z = z.begin();

		for (register int i = 0; i < R_max_index; i++)//подвинусь до нужного отрезка
		{
			iter_z++;
			iter_x++;
		}

		// сохранение
		curr_right = *iter_x;
		curr_left = *--iter_x;

		//сбор данных 
		p.X.push_back(curr_right);
		p.steps++;

		if (abs(curr_left - curr_right) < _Eps) { break; } //условие выхода

		localSteps++;
		if (localSteps + 1 > _N_max) { break; }//условие выхода



	}

	end = omp_get_wtime();
	//сбор данных 

	p.time = end - start;
	p.x = curr_right;
	p.z = *iter_z;


	return p;
}

