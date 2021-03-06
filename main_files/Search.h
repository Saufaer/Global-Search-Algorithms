#ifndef __SEARCH_H__
#define __SEARCH_H__

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <queue>
#include <list>

#include <omp.h>

struct Pointer//общая структура для сбора результатов
{
	double x, z;
	int steps;
	std::vector<double> X;
	double time;
};

struct Group//рабочая структура для ПП по хар-кам

{
	double R; // оценка константы Липшица
	double M; // параметр оценки для интервала
	double x_left; // координата слева
	double x_right; // координата справа
	double z_left; // значение слева
	double z_right; // значение справа
};


struct borders//рабочая структура для ПП по отрезкам
{
	double left;
	double right;
};

class Search
{
private:
	double left; // левая граница
	double right; // правая граница
	double r; // параметр метода
	int procs;//число потоков

	//(Характеристики)векторы для общей работы потоков 
	std::list<Group> list_arr_x;
	std::list<Group> list_arr_z;

	//(Отрезки)векторы для сбора результатов с потоков
	std::vector<std::list<double>> parallel_z;
	std::vector<std::list<double>> parallel_x;
public:



	Search(const double _left, const double _right, const double _r, const int _procs)//конструктор
	{
		left = _left;
		right = _right;
		r = _r;
		procs = _procs;
	}

	double Func(const double &_x)//целевая функция
	{
		//return sin(_x) + sin((10. * _x) / 3.); // (2.7 , 7.5 , r = 4.29 )
		//return (2 * pow((_x - 3), 2) + exp((pow(_x, 2) / 2))); // (-3, 3, r = 85)
		//return ((3 * _x - 1.4)*sin(18 * _x)); // //(0, 1.2 , r = 36)
		//return (sin(_x) + sin((10. * _x) / 3.) + log(_x) - 0.84*_x + 3); //(2.7 , 7.5 , r = 6)
		//return ((pow(_x, 2) - 5 * _x + 6) / (pow(_x, 2) + 1)); //(-5, 5 , r = 6.5)
		return 2 * sin(10 * _x) + 3 * cos(11 * _x); // 
	}



	Pointer Serial_searchMin(const double _left, const double _right, const int _N_max, const double _Eps);//Последовательный поиск

	Pointer Simple_Par_searchMin(const double _left, const double _right, const int _N_max, const double _Eps, const int _threads);//ПП по отрезкам

	Pointer Ch_SearchMin(const double _Epsilon, const int _Steps, const int _thread_count);//ПП по характеристикам

	double R(const double &_m_small, const double &_z_curr, const double &_z_prev, const double &_x_curr, const double &_x_prev)//Характеристика
	{
		return _m_small * (_x_curr - _x_prev) + pow(_z_curr - _z_prev, 2) / (_m_small*(_x_curr - _x_prev)) - 2 * (_z_curr + _z_prev);
	};

	double M(const double &_z_curr, const double &_z_prev, const double &_x_curr, const double &_x_prev)//Параметр для оценки константы Липшица
	{
		return abs((_z_curr - _z_prev) / (_x_curr - _x_prev));
	};

	double New_x(double &x_right, double &x_left, double &z_right, double &z_left, double &m_small)
	{

		return  (x_right + x_left) / 2 - (z_right - z_left) / (2 * m_small);
	}
	void Lipschitz(double &M_big, double &m_small)
	{
		if (M_big == 0)
		{
			m_small = 1;
		}
		else
		{
			m_small = r * M_big;
		}
	}

};


#endif

