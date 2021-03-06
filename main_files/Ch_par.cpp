
#include "Search.h"



Pointer Search::Ch_SearchMin(const double _Epsilon, const int _Steps, const int _thread_count)
{
	Pointer p;
	p.steps = 0;
	p.time = 0;

	int procs = _thread_count;

	int localSteps = 0; // шаги для внутреннего счета

	double eps = _Epsilon;

	//текущие границы
	double curr_left;
	double curr_right;

	// время работы
	double start, end;

	double *x_new = new double[procs];

	//константы для оценки конст Липшеца
	double m_small = 1;
	double M_big;

	//границы
	curr_left = left;
	curr_right = right;

	std::vector<Group> StartVec(procs);//вектор для первой итерации

	std::vector<Group> MainVec(procs * 2);//вектор для остальных итераций

	std::list<Group> Local_group;

	size_t size_group; // размер list<Group> Local_group

	std::list<Group>::iterator Pointer;

	omp_set_dynamic(0);//среда выполнения не будет динамически настроить количество потоков

	start = omp_get_wtime();

	omp_set_num_threads(procs);//установим число нужных потоков
	{
#pragma omp parallel for firstprivate(tmp) num_threads(procs)//firstprivate(tmp) - данные для отдельной копии в памяти каждого потока
		for (int i = 0; i < procs; i++)//распределение отрезка по потокам
		{
			StartVec[i].x_left = left + (i * (right - left) / procs);
			StartVec[i].x_right = StartVec[i].x_left + (right - left) / procs;
			StartVec[i].z_left = Func(StartVec[i].x_left);
			StartVec[i].z_right = Func(StartVec[i].x_right);
		}

		// подсчет M
#pragma omp parallel for num_threads(procs)
		for (int i = 0; i < procs; i++)
		{
			StartVec[i].M = M(StartVec[i].z_right, StartVec[i].z_left, StartVec[i].x_right, StartVec[i].x_left);
		}

		M_big = StartVec[0].M;//найденная М одним потоком

		for (int i = 1; i < procs; i++) // найдем макс из остальных
		{
			if (M_big < StartVec[i].M)
			{
				M_big = StartVec[i].M;
			}
		}
		Lipschitz(M_big, m_small);// оценка параметра m

		//подсчёт R
#pragma omp parallel for num_threads(procs)
		for (int i = 0; i < procs; i++)
		{
			StartVec[i].R = R(m_small, StartVec[i].z_right, StartVec[i].z_left, StartVec[i].x_right, StartVec[i].x_left);
		}
		for (register int i = 0; i < procs; i++)//вставка результатов в общий список
		{
			Local_group.push_back(StartVec[i]);
		}
		Local_group.sort();//сортировка по возрастанию

		//получение текущих границ
		curr_left = Local_group.back().x_left;
		curr_right = Local_group.back().x_right;

		localSteps++;

		while (true)//основной цикл
		{
			Pointer = --Local_group.end();
			for (int i = 0; i < procs * 2 - 2; i += 2) // хватаем proc_count R-ок (самых больших)
			{
				MainVec[i] = *Pointer;
				Local_group.erase(Pointer--);//остальные R-ки стираем (все группы)
			}
			MainVec[procs * 2 - 2] = *Pointer;
			Local_group.erase(Pointer);//.erase сотрет по указателю
			size_group = Local_group.size();

			// соответствующей  кучке больших R-ок находим соответствующие точки испытаний
#pragma omp parallel for shared(x_new) num_threads(procs)
			for (int i = 0; i < procs; i++)
			{
				x_new[i] = New_x(MainVec[i * 2].x_right, MainVec[i * 2].x_left, MainVec[i * 2].z_right, MainVec[i * 2].z_left, m_small);


				MainVec[i * 2 + 1].z_left = Func(x_new[i]);
			}

			for (register int i = 0; i < procs * 2; i += 2)//сохраняем найденные точки
			{
				MainVec[i + 1].x_right = MainVec[i].x_right;
				MainVec[i + 1].z_right = MainVec[i].z_right;
				MainVec[i + 1].x_left = MainVec[i].x_right = x_new[i / 2];
				MainVec[i].z_right = MainVec[i + 1].z_left;
			}

			// считаем М
#pragma omp parallel for num_threads(procs)
			for (int i = 0; i < procs * 2; i++)
			{
				MainVec[i].M = M(MainVec[i].z_right, MainVec[i].z_left, MainVec[i].x_right, MainVec[i].x_left);
			}

			double M_max_all_M = MainVec[0].M;//найденная М одним потоком




				//поиск наибольшего M, который не соответсвует текущей кучке(из M_max_all_M) (а он может быть большим, чем все M из кучки)
			for (register int i = 1; i < procs * 2; i++)
			{
				if (M_max_all_M < MainVec[i].M)
				{
					M_max_all_M = MainVec[i].M;
				}
			}
			// поиск наибольшего M из кучки (из M_max_cogorta)
			if (size_group != 0)
			{
				Pointer = Local_group.begin();
				double tmp_M;
				double M_max_cogorta = (*Pointer++).M;
				while (Pointer != Local_group.end())
				{
					tmp_M = (*Pointer).M;
					if (M_max_cogorta < tmp_M) M_max_cogorta = tmp_M;

					Pointer++;
				}

				//поиск большего M среди найденных
				if (M_max_all_M < M_max_cogorta)
				{
					M_big = M_max_cogorta;
				}
				else
				{
					M_big = M_max_all_M;;
				}

			}
			else
			{
				M_big = M_max_all_M;
			}

			//подсчёт m 
			Lipschitz(M_big, m_small);

#pragma omp parallel for num_threads(procs)
			for (int i = 0; i < procs * 2; i++)//считаем R для текущих отрезков
			{
				MainVec[i].R = R(m_small, MainVec[i].z_right, MainVec[i].z_left, MainVec[i].x_right, MainVec[i].x_left);
			}


			if (size_group != 0)
			{
				std::vector<Group> R_vec(Local_group.begin(), Local_group.end());

#pragma omp parallel for num_threads(procs)
				for (int i = 0; i < size_group; i++)//подсчёт R для отрезков  из очереди
				{
					R_vec[i].R = R(m_small, R_vec[i].z_right, R_vec[i].z_left, R_vec[i].x_right, R_vec[i].x_left);
				}
				int j = 0;
				for (std::list<Group>::iterator i = Local_group.begin(); i != Local_group.end(); i++)
				{
					i->R = R_vec[j++].R;
				}
				R_vec.clear();
			}
			for (int i = 0; i < procs * 2; i++)
			{

				Local_group.push_back(MainVec[i]);//кладем новые группы в общий список
			}

			//сбор данных 
			p.X.push_back(Local_group.back().x_right);//.back() возвращает ссылку на последний элемент в контейнере списка 
			p.steps++;

			Local_group.sort();

			localSteps++;
			if (localSteps > _Steps) { break; }//условие остановки

			curr_left = Local_group.back().x_left;
			curr_right = Local_group.back().x_right;
			if (abs(curr_left - curr_right) < eps) { break; }//условие остановки

		}
	}

	end = omp_get_wtime();
	//сбор данных 
	p.time = end - start;

	omp_set_dynamic(1);


	//сбор данных 
	p.X.push_back(Local_group.back().x_right);
	p.x = curr_right;
	p.z = Local_group.back().z_right;


	// очистка
	StartVec.clear();
	MainVec.clear();
	delete[] x_new;

	return p;
}

bool operator<(const Group& one, const Group& two)
{
	if (one.R < two.R)
	{
		return true;
	}
	else
	{
		return false;
	}

};

bool operator>(const Group& one, const Group& two)
{
	if (one.R > two.R)
	{
		return true;
	}
	else
	{
		return false;
	}
};