/*
 * main.cpp
 *
 *  Created on: Jan 10, 2013
 *      Author: ykirpichev
 *
 *      In this assignment you will implement one or more algorithms
 *      for the all-pairs shortest-path problem.
 *      Here are data files describing three graphs:
 *       graph #1; graph #2; graph #3.
 *      The first line indicates the number of vertices and edges, respectively.
 *      Each subsequent line describes an edge
 *      (the first two numbers are its tail and head, respectively)
 *      and its length (the third number).
 *      NOTE: some of the edge lengths are negative.
 *      NOTE: These graphs may or may not have negative-cost cycles.
 *
 *      Your task is to compute the "shortest shortest path".
 *      Precisely, you must first identify which, if any, of the three graphs
 *      have no negative cycles.
 *      For each such graph, you should compute all-pairs shortest paths and
 *      remember the smallest one
 *      (i.e., compute minu,v∈Vd(u,v),
 *       where d(u,v) denotes the shortest-path distance from u to v).
 *
 *       If each of the three graphs has a negative-cost cycle,
 *       then enter "NULL" in the box below.
 *       If exactly one graph has no negative-cost cycles,
 *       then enter the length of its shortest shortest path in the box below.
 *       If two or more of the graphs have no negative-cost cycles,
 *       then enter the smallest of the lengths of their shortest shortest paths
 *       in the box below.
 *
 *       OPTIONAL: You can use whatever algorithm you like to solve this question.
 *       If you have extra time,
 *       try comparing the performance of different all-pairs shortest-path
 *       algorithms!
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <chrono>
#include <iomanip>
using namespace std;

typedef vector<int> vi;
typedef vector<tuple<int, int, int> > viii;
typedef vector<pair<int, int> > vii;
int N, M;
viii edges;
vector<vii> G, RG;
vi belman_ford();
vi deijkstra(int s, vector<vii> g);
pair<int, int> gen_pair()
{
    static int count = 1;
    return make_pair(count++, 0);
}
long long L[1000 + 1][1000 + 1][2];

void floyd_warshall()
{


    for (int i = 1; i <= N; ++i)
    {
        for (int j = 1; j <= N; ++j)
        {
            L[i][j][0] = numeric_limits<long long>::max();
        }
        L[i][i][0] = 0;
    }

    for (auto ed = edges.begin(); ed != edges.end(); ++ed)
    {
        L[get<0>(*ed)][get<1>(*ed)][0] = get<2>(*ed);
    }

    int current = 1;
    for (int k = 1; k < N + 1; ++k )
    {
        for (int i = 1; i <= N; ++i)
        {
            for (int j = 1; j <= N; ++j)
            {
                if (L[i][k][1 - current] != numeric_limits<long long>::max() &&
                        L[k][j][1 - current] != numeric_limits<long long>::max())
                {
                    L[i][j][current] = min(L[i][j][1-current], L[i][k][1 - current] + L[k][j][1 - current]);
                }
                else
                {
                    L[i][j][current] = L[i][j][1-current];
                }
            }
        }
        current = 1 - current;
    }

    int last = 1 - current;
    long long min = numeric_limits<long long>::max();
    for (int i = 1; i <= N; ++i)
    {
        for (int j = 1; j <= N; ++j)
        {
            if (L[i][j][last] < min)
            {
                min = L[i][j][last];
            }
        }
    }

    cout <<  "floyd_warshall: min = " << min << endl;
}

void jonson()
{
	vi d = belman_ford();
	// update weigths
	// compute n deijkstas
	// get original length
	for (uint u = 1; u < G.size(); ++u)
	{
		for (auto val = G[u].begin(); val != G[u].end(); ++val)
		{
		// c + u - v
			int v = val->first;

			val->second += d[u] - d[v];
		}

	}

	int m = numeric_limits<int>::max();
	for (int i = 1; i < N + 1; ++i)
	{
		vi res = deijkstra(i, G);

		for (int v = 1; v < N + 1; ++v)
		{
			res[v] += d[v] - d[i];
		}

		auto it = min_element(res.begin() + 1, res.end());
		if (*it < m)
		{
			m = *it;
		}
//		cout << *m << endl;
	}
	cout << "jonson: min = " << m << endl;
}
vi deijkstra(int s, vector<vii> g)
{
	set<pair<int, int>> q;
	vi d(g.size(), numeric_limits<int>::max());

	d[s] = 0;
	q.insert(make_pair(0, s));

	while (!q.empty())
	{
		auto top = *q.begin();
		int u = top.second;
		q.erase(q.begin());

		for (auto other = g[u].begin(); other != g[u].end(); ++other)
		{
			int w = other->second;
			int v = other->first;
			int tmp = d[u] + w;

			if (tmp < d[v])
			{
				if (q.find(make_pair(d[v], v)) != q.end())
				{
					q.erase(q.find(make_pair(d[v], v)));
				}
				d[v] = d[u] + w;
				q.insert(make_pair(d[v], v));
			}
		}
	}

	return d;
}

vi belman_ford()
{
    long long L[N + 1][N + 1];
    bool changed = false;

    // start point is 0
    // initialize 0 as
    L[0][0] = 0;
    for (int i = 1; i < N + 1; ++i)
    {
        L[i][0] = numeric_limits<long long>::max();
    }
    int i;
    for (i = 1; i <= N; ++i)
    {
        changed = false;
        for (int k = 0; k <= N; ++k)
        {
            L[i][k] = L[i - 1][k];
            for (auto v = RG[k].begin(); v != RG[k].end(); ++v)
            {
                long long val = L[i - 1][v->first] + v->second;
                if (val < L[i][k])
                {
                    L[i][k] = val;
                    changed = true;
                }
            }
        }

        if (!changed)
        {
            break;
        }
    }

    if (changed)
    {
        cout << "there is negative circle" << endl;
        throw logic_error("there is negative circle");
    }

    cout << "no negative circle found" << endl;
    //create vector with paths
    vi d(N + 1);
    copy(&L[i][0], &L[i][0] + N + 1, d.begin());

    return d;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cout << "Usage: program <file_name>" << endl;
    }

    ifstream ifs(argv[1]);

    ifs >> N >> M;
    edges.reserve(M);
    G = vector<vii>(N + 1);
    RG = vector<vii>(N + 1);

    for (int i = 0; i < M; ++i)
    {
        int v1, v2, w;

        ifs >> v1 >> v2 >> w;

        edges.push_back(make_tuple(v1, v2, w));
        G[v1].push_back(make_pair(v2, w));
        RG[v2].push_back(make_pair(v1, w));
    }

    // first implement belman-ford and check whether negative weight circle exists
    // then reweight and run n times diejkstra algorithm

    // add zero weight edge from vertex(0) to other verticies
    G[0].resize(N);
    generate_n(G[0].begin(), N , gen_pair);
    for (int i = 1; i <= N; ++i)
    {
        RG[i].push_back(make_pair(0, 0));
    }
//            [](){
//                static int count = 0;
//                ++count;
//                return make_pair(count, 0);
//            });

//    for (int i = 1; i <= N; ++i)
//    {
//        G[0].push_back(make_pair(i, 0));
//    }
//    belman_ford();

    auto start1 = std::chrono::high_resolution_clock::now();
    floyd_warshall();
    auto end1 = std::chrono::high_resolution_clock::now();
    jonson();
    auto end2 = std::chrono::high_resolution_clock::now();

    auto elapsed1 = end1 - start1;
    auto elapsed2 = end2 - end1;
    using std::chrono::duration_cast;
    using std::chrono::nanoseconds;

    cout << setw(25) << left << "floyd_warshall time : " << setw(15) << right << std::chrono::duration_cast<nanoseconds>(elapsed1).count() << endl;
    cout << setw(25) << left << "jonson time : " << setw(15) << right << std::chrono::duration_cast<nanoseconds>(elapsed2).count() << endl;
}
