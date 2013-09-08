/*
 * main.cpp
 *
 *  Created on: Dec 13, 2012
 *      Author: ykirpichev
 */

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
using namespace std;

int N; // number of nodes
int M; // number of bits

typedef vector<long long> vl;





class DisjointSet
{
	struct unionNode
	{
		unionNode(int p = 0, int rank = 0): p(p), rank(rank) {}
		int p;
		int rank;
	};
	typedef vector<unionNode> vUN;

	vUN data;
	int numberofcomponent;
public:
	DisjointSet(int N) :
			data(N), numberofcomponent(N)
	{
		for (int i = 0; i < N; ++i)
		{
			data[i].p = i;
		}
	}

	int findSet(int i)
	{
		if (i != data[i].p)
		{
			data[i].p = findSet(data[i].p);
		}

		return data[i].p;
	}

	void unionLink(int i, int j)
	{
		if (data[i].rank > data[j].rank)
		{
			data[j].p = i;
		}
		else
		{
			data[i].p = j;
			if (data[i].rank == data[j].rank)
			{
				++data[j].rank;
			}
		}
	}

	void unionNodes(int i, int j)
	{
		unionLink(findSet(i), findSet(j));
		--numberofcomponent;
	}

	int getNumberOfComponent(void)
	{
		return numberofcomponent;
	}
};

long long getlonglong(string str)
{
	stringstream ss(str);
	long long result = 0;
	long long tmp = 0;

	while(ss >> tmp)
	{
		result = (result << 1) + tmp;
	}

	return result;
}

struct CmpByMask
{
	CmpByMask(long long mask) : mask(mask) {}
	bool operator()(long long lhs, long long rhs)
	{
		return ((lhs & mask) < (rhs & mask));
	}
	long long mask;
};

void unionEquals(vl & G, DisjointSet &ds, CmpByMask cmp)
{
	for (uint i = 0; i < G.size() - 1; ++i)
	{
		/* if previous less than next */
		if (cmp(G[i], G[i + 1]))
		{
			continue;
		}

		/* two adjacent nodes are equal */
		/* check if they belong to different component and union them */
		/* BUG: keep index in G */
		if (ds.findSet(i) != ds.findSet(i + 1))
		{
			ds.unionNodes(i, i + 1);
		}
	}
}

int main(int argc, char **argv)
{
	ifstream ifs;

	if (argc != 2)
	{
		cout << getlonglong("1 1 1 1") << endl;
		cout << getlonglong("1 0 1 0") << endl;
		return -1;
	}

	ifs.open(argv[1]);
	string str;
	ifs >> N >> M;
	getline(ifs, str);
	vl G = vl(N);
	DisjointSet ds = DisjointSet(N);

	for (int i = 0; i < N; ++i)
	{
		std::getline(ifs, str);
		G[i] = getlonglong(str);
	}

	sort(G.begin(), G.end());
	unionEquals(G, ds, CmpByMask(-1));

	/* find all nodes, which differ by single bit only */
	for (int i = 0; i < M; ++i)
	{
		long long mask = ~(1LL << i);
		sort(G.begin(), G.end(), CmpByMask(mask));
		unionEquals(G, ds, CmpByMask(mask));
	}
	/* find all nodes, which differ by two bits */
	for (int i = 0; i < M - 1; ++i)
	{
		for (int j = i + 1; j < M; ++j)
		{
			long long mask = ~((1LL << i) + (1LL << j));
			sort(G.begin(), G.end(), CmpByMask(mask));
			unionEquals(G, ds, CmpByMask(mask));
		}
	}
	/* output how many clusters do we have */
	cout << ds.getNumberOfComponent() << endl;

	return 0;
}
