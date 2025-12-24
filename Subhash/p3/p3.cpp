#include <iostream>
#include <vector>
#include <stack>
#include <set>
using namespace std;

/*
 MODEL 3: Critical Infrastructure / Sanitation Network
 Using Tarjan's Bridge Finding Algorithm
*/

// ---------- GLOBAL STRUCTURES ----------
vector<vector<int>> graph;     // Graph (Adjacency List)
vector<int> disc, low;         // Discovery & Low-link arrays
vector<bool> visited;          // Visited array
set<pair<int,int>> bridges;    // Store unique bridges
stack<int> dfsStack;           // Stack (DFS tracking)
int timer = 0;

// ---------- DFS + TARJAN LOGIC ----------
void dfsBridge(int u, int parent) {
    visited[u] = true;
    disc[u] = low[u] = timer++;

    dfsStack.push(u); // Stack usage

    for (int v : graph[u]) {

        if (v == parent)
            continue;

        if (!visited[v]) {
            dfsBridge(v, u);

            // Update low-link value
            low[u] = min(low[u], low[v]);

            // Bridge condition
            if (low[v] > disc[u]) {
                bridges.insert({u, v});
            }
        }
        else {
            // Back edge case
            low[u] = min(low[u], disc[v]);
        }
    }

    dfsStack.pop(); // Stack pop
}

// ---------- DRIVER ----------
int main() {
    int V = 7;
    graph.resize(V);
    disc.resize(V, -1);
    low.resize(V, -1);
    visited.resize(V, false);

    // ----- Infrastructure Connections (Edges) -----
    graph[0].push_back(1);
    graph[1].push_back(0);

    graph[1].push_back(2);
    graph[2].push_back(1);

    graph[2].push_back(0);
    graph[0].push_back(2);

    graph[1].push_back(3);
    graph[3].push_back(1);

    graph[3].push_back(4);
    graph[4].push_back(3);

    graph[4].push_back(5);
    graph[5].push_back(4);

    graph[5].push_back(6);
    graph[6].push_back(5);

    // ----- Run DFS for all components -----
    for (int i = 0; i < V; i++) {
        if (!visited[i]) {
            dfsBridge(i, -1);
        }
    }

    // ----- Output Bridges -----
    cout << "Critical Infrastructure Bridges:\n";
    for (auto b : bridges) {
        cout << b.first << " - " << b.second << endl;
    }

    return 0;
}

