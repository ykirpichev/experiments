/*
 * disjointset.h
 *
 *  Created on: Dec 14, 2012
 *      Author: k.yury
 */

#ifndef DISJOINTSET_H_
#define DISJOINTSET_H_

#include <vector>
#include <iostream>

class DisjointSet
{
	struct unionNode
	{
		unionNode(int p = 0, int rank = 0): p(p), rank(rank) {}
		int p;
		int rank;
	};
	typedef std::vector<unionNode> vUN;

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

	void dumpComponents()
	{
		for (uint i = 0; i < data.size(); ++i)
		{
			std::cout << i << " --> " << findSet(i) << " rank:" << data[i].rank << std::endl;
		}
	}
};


#endif /* DISJOINTSET_H_ */
