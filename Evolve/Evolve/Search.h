#ifndef __SEARCH_H__
#define __SEARCH_H__

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <queue>
#include <list>
#include <omp.h>

#include "grishagin\include\grishagin_function.hpp"
#include "evolvent.h"

struct PointerOneDim//структура для сбора результатов одномерной задачи
{
	double x, z;
	int steps;
	std::vector<double> X;
	double time;
};

struct PointerTwoDim//структура для сбора результатов двумерной задачи
{
	double x, y, z;
	int steps;
	std::vector<double> X;
	std::vector<double> Y;
	double time;
};

struct GroupOneDim//рабочая структура для ПП одномерной задачи по хар-кам
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

struct GroupTwoDim//рабочая структура для двумерной задачи
{
	double R; // оценка константы Липшица
	double M; // параметр оценки для интервала

	//интервальные точки на единичном отрезке 
	double x_new_left;
	double x_new_right;

	double x_left; // x координата слева
	double x_right; // x координата справа
	double y_left; // y координата слева
	double y_right; // y координата справа
	double z_left; // значение слева
	double z_right; // значение справа

};

class Search
{
private:
	double left; // левая граница одномерной задачи
	double right; // правая граница  одномерной задачи

	double* Left; // ограничение области слева двумерной задачи
	double* Right; // ограничение области справа двумерной задачи

	double r; // параметр метода
	int procs;//число потоков

public:
	Search(const double _left, const double _right, const double _r, const int _procs)//конструктор для одномерной задачи
	{
		left = _left;
		right = _right;
		r = _r;
		procs = _procs;
	}

	Search(const double *_left, const double *_right, const double _r, const int _procs)//конструктор для двумерной задачи
	{
		Left = new double[2];
		Right = new double[2];
		for (int i = 0; i < 2; i++)
		{
			Left[i] = _left[i];
			Right[i] = _right[i];
		}
		r = _r;
		procs = _procs;
	}

	double Func(const double &_x)//Одномерные функции
	{
		//return sin(_x) + sin((10. * _x) / 3.); // (2.7 , 7.5 , r = 4.29 )
		//return (2 * pow((_x - 3), 2) + exp((pow(_x, 2) / 2))); // (-3, 3, r = 85)
		//return ((3 * _x - 1.4)*sin(18 * _x)); // //(0, 1.2 , r = 36)
		//return (sin(_x) + sin((10. * _x) / 3.) + log(_x) - 0.84*_x + 3); //(2.7 , 7.5 , r = 6)
		//return ((pow(_x, 2) - 5 * _x + 6) / (pow(_x, 2) + 1)); //(-5, 5 , r = 6.5)
		return 2 * sin(10 * _x) + 3 * cos(11 * _x); // 
	}

	double Func(const double *_y, vagrish::GrishaginFunction *func) //Значение двумерной функции Гришагина
	{
		return func->Calculate(_y);
	}

	PointerOneDim Serial_searchMin(const double _left, const double _right, const int _N_max, const double _Eps);//Последовательный поиск одномерной задачи

	PointerOneDim Simple_Par_searchMin(const double _left, const double _right, const int _N_max, const double _Eps, const int _threads);//ПП по отрезкам одномерной задачи

	PointerOneDim Ch_SearchMin(const double _Epsilon, const int _Steps, const int threads);//ПП по характеристикам одномерной задачи

	PointerTwoDim Two_Dim_Search(const double _Epsilon, const int _Steps, const int threads, vagrish::GrishaginFunction *func);//Решатель двумерной задачи

	double R(const double &_m_small, const double &_z_curr, const double &_z_prev, const double &_x_curr, const double &_x_prev)//Характеристика
	{
		return _m_small * (_x_curr - _x_prev) + pow(_z_curr - _z_prev, 2) / (_m_small*(_x_curr - _x_prev)) - 2 * (_z_curr + _z_prev);
	};

	double M(const double &_z_curr, const double &_z_prev, const double &_x_curr, const double &_x_prev)//Параметр для оценки константы Липшица
	{
		return abs((_z_curr - _z_prev) / (_x_curr - _x_prev));
	};

	double New_x(double &x_right, double &x_left, double &z_right, double &z_left, double &m_small)//Новая точка испытания
	{

		return  (x_right + x_left) / 2 - (z_right - z_left) / (2 * m_small);
	}
	void Lipschitz(double &M_big, double &m_small)//Оценка константы Липшеца
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

