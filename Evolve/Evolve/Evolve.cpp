#include "Search.h"
#include "evolv.h"
#include <iostream>
using namespace std;
int main()
{
	system("color F0");
	Pointer p1;
	evolv Y;
	double left = 0;
		double right = 1;
		double r = 2;
		double eps = 0.0001;

		int n_max = 1000;
		int threads = 1;
		
		const int dim = Y.mDimension;
		std::vector<double> curY ;
		//for (int i = 0; i < NumPoints; i++)
		//{
		//	// Вычисляем образ точки итерации - образ записывается в массив y, начиная с индекса pTask->GetFixedN()
		//	//TEvolvent->GetImage(x[i], pCurTrials[i].y + pTask->GetFixedN());
		//}
	Search ptest1(left, right, r, threads);

	p1 = ptest1.Ch_SearchMin(eps, n_max, threads);
	double y[MaxDim];
	for (int i = 0; i < p1.X.size(); i++)
	{
		Y.GetImage(p1.X[i], y, 0);
	 curY.push_back(y[0]);
	}
	for (int i = 0; i < p1.X.size(); i++)
	{
		cout << curY[i] << " | ";
	}
	cout << endl << p1.steps*threads << endl;
	cout << p1.z << endl;
	cout << p1.x << endl;
	cout << p1.time << endl;


    return 0;
}

