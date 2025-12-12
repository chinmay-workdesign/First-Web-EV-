#include <bits/stdc++.h>
using namespace std;

// Dijkstra's algorithm for shortest-time path in a directed, weighted graph.
// Input CSV format (first line header):
// num_nodes,num_edges,start,end
// Then num_edges lines: u,v,weight
// Nodes are 0-indexed integers in [0, num_nodes-1].

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // Read header line
    string header;
    if (!getline(cin, header)) return 0;

    // Parse header
    stringstream ss(header);
    int n, m, start, target;
    char comma;
    ss >> n >> comma >> m >> comma >> start >> comma >> target;

    vector<vector<pair<int,int>>> adj(n);

    // Read m edge lines
    for (int i = 0; i < m; ++i) {
        string line;
        if (!getline(cin, line)) break;
        if (line.size() == 0) { --i; continue; }
        stringstream es(line);
        int u, v, w;
        char c;
        es >> u >> c >> v >> c >> w;
        if (u < 0 || u >= n || v < 0 || v >= n) continue;
        adj[u].push_back({v, w});
    }

    const long long INF = (1LL<<60);
    vector<long long> dist(n, INF);
    vector<int> parent(n, -1);

    // Min-heap: pair(distance, node)
    priority_queue<pair<long long,int>, vector<pair<long long,int>>, greater<pair<long long,int>>> pq;
    dist[start] = 0;
    pq.push({0, start});

    while (!pq.empty()) {
        auto [d,u] = pq.top(); pq.pop();
        if (d != dist[u]) continue;
        if (u == target) break; // optional early exit
        for (auto &e : adj[u]) {
            int v = e.first;
            int w = e.second;
            if (dist[v] > dist[u] + w) {
                dist[v] = dist[u] + w;
                parent[v] = u;
                pq.push({dist[v], v});
            }
        }
    }

    if (dist[target] == INF) {
        cout << "unreachable\n";
        return 0;
    }

    // Reconstruct path
    vector<int> path;
    for (int cur = target; cur != -1; cur = parent[cur]) path.push_back(cur);
    reverse(path.begin(), path.end());

    cout << "Total time: " << dist[target] << "\n";
    cout << "Path (size " << path.size() << "):\n";
    for (size_t i = 0; i < path.size(); ++i) {
        if (i) cout << " -> ";
        cout << path[i];
    }
    cout << "\n";

    return 0;
}
