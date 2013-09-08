/*
 * main.cpp
 *
 *  Created on: Dec 14, 2012
 *      Author: k.yury
 */

#include <iostream>
#include <vector>
#include <fstream>
#include <limits>
#include <algorithm>
#include "disjointset.h"
using namespace std;

struct Edge
{
	int w, v1, v2;

	Edge(int w = 0, int v1 = 0, int v2 = 0): w(w), v1(v1), v2(v2) {}
	bool operator<(const Edge o) const
	{
		return w < o.w;
	}
};

typedef vector<Edge> VE;

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		cout << "please provide file name with input data\n";
		return -1;
	}

	ifstream ifs(argv[1]);
	int N;
	VE edges;
	ifs >> N;
	ifs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	int w, v1, v2;
	edges.reserve(N * N / 2);
	while (ifs >> v1 >> v2 >> w)
	{
		--v1, --v2;
		edges.push_back(Edge(w, v1, v2));
	}

	sort(edges.begin(), edges.end());
	DisjointSet dset(N);

	uint i = 0;
	int space = 0;
	for (i = 0; i < edges.size(); ++i)
	{
		if (dset.getNumberOfComponent() == 4)
		{
			break;
		}
		if (dset.findSet(edges[i].v1) != dset.findSet(edges[i].v2))
		{
			dset.unionNodes(edges[i].v1, edges[i].v2);
		}
	}
	for (; i < edges.size(); ++i)
	{
		if (dset.findSet(edges[i].v1) != dset.findSet(edges[i].v2))
		{
			space = edges[i].w;
			break;
		}
	}
	dset.dumpComponents();
	cout << space << endl;
	return 0;
}
