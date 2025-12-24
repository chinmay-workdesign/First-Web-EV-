#include <iostream>
#include <vector>
#include <queue>
#include <climits>
using namespace std;

/* ---------------- UNION FIND (DSU) ---------------- */
class UnionFind {
    vector<int> parent, rankv;
public:
    UnionFind(int n) {
        parent.resize(n);
        rankv.resize(n, 0);
        for (int i = 0; i < n; i++)
            parent[i] = i;
    }

    int find(int x) {
        if (parent[x] != x)
            parent[x] = find(parent[x]);
        return parent[x];
    }

    void unite(int a, int b) {
        a = find(a);
        b = find(b);
        if (a != b) {
            if (rankv[a] < rankv[b])
                parent[a] = b;
            else if (rankv[a] > rankv[b])
                parent[b] = a;
            else {
                parent[b] = a;
                rankv[a]++;
            }
        }
    }
};

/* ---------------- BFS FOR CONNECTIVITY ---------------- */
void BFS(int start, vector<vector<int>>& adj, vector<bool>& visited) {
    queue<int> q;
    q.push(start);
    visited[start] = true;

    while (!q.empty()) {
        int u = q.front();
        q.pop();
        for (int v : adj[u]) {
            if (!visited[v]) {
                visited[v] = true;
                q.push(v);
            }
        }
    }
}

/* ---------------- DIJKSTRA WITH MIN HEAP ---------------- */
vector<int> dijkstra(int src, vector<vector<pair<int,int>>>& graph) {
    int n = graph.size();
    vector<int> dist(n, INT_MAX);

    priority_queue<
        pair<int,int>,
        vector<pair<int,int>>,
        greater<pair<int,int>>
    > minHeap;

    dist[src] = 0;
    minHeap.push({0, src});

    while (!minHeap.empty()) {
        int u = minHeap.top().second;
        int d = minHeap.top().first;
        minHeap.pop();

        if (d > dist[u]) continue;

        for (auto edge : graph[u]) {
            int v = edge.first;
            int w = edge.second;   // congestion weight

            // Edge relaxation
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                minHeap.push({dist[v], v});
            }
        }
    }
    return dist;
}

/* ---------------- MAIN ---------------- */
int main() {
    int n = 6;  // number of intersections

    /* Graph for BFS (connectivity check) */
    vector<vector<int>> adj(n);

    /* Graph for Dijkstra (weighted congestion) */
    vector<vector<pair<int,int>>> graph(n);

    /* Road network */
    adj[0] = {1};
    adj[1] = {0,2};
    adj[2] = {1};
    adj[3] = {4};
    adj[4] = {3};
    adj[5] = {};   // isolated node

    graph[0].push_back({1,5});
    graph[1].push_back({2,2});
    graph[3].push_back({4,1});

    /* ---------------- BFS: Detect poor connectivity ---------------- */
    vector<bool> visited(n, false);
    BFS(0, adj, visited);

    cout << "Unreachable zones (poor connectivity): ";
    for (int i = 0; i < n; i++) {
        if (!visited[i])
            cout << i << " ";
    }
    cout << "\n";

    /* ---------------- UNION FIND: Detect separated clusters ---------------- */
    UnionFind uf(n);
    for (int u = 0; u < n; u++) {
        for (int v : adj[u]) {
            uf.unite(u, v);
        }
    }

    cout << "Urban clusters (DSU):\n";
    for (int i = 0; i < n; i++) {
        cout << "Node " << i << " -> Cluster " << uf.find(i) << "\n";
    }

    /* ---------------- DIJKSTRA: Congestion-weighted routes ---------------- */
    vector<int> dist = dijkstra(0, graph);

    cout << "Congestion-weighted distances from node 0:\n";
    for (int i = 0; i < n; i++) {
        if (dist[i] == INT_MAX)
            cout << "Node " << i << ": Unreachable\n";
        else
            cout << "Node " << i << ": " << dist[i] << "\n";
    }

    return 0;
}
