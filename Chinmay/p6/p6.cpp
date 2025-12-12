#include <bits/stdc++.h>
using namespace std;

// Tarjan's Bridge-Finding algorithm for undirected graphs.
// Input CSV format (first line header):
// num_nodes,num_edges
// Second line: N,M
// Next M lines: u,v  (0-indexed undirected edges)

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string header;
    if (!getline(cin, header)) return 0;
    stringstream ss(header);
    int n, m; char c;
    ss >> n >> c >> m; // parse header

    string line;
    if (!getline(cin, line)) return 0; // second line with n,m
    stringstream s2(line);
    s2 >> n >> c >> m;

    vector<vector<pair<int,int>>> adj(n); // neighbor, edge id
    vector<pair<int,int>> edges;
    edges.reserve(m);

    for (int i = 0; i < m; ++i) {
        if (!getline(cin, line)) break;
        if (line.size() == 0) { --i; continue; }
        stringstream es(line);
        int u, v; char cc;
        es >> u >> cc >> v;
        if (u < 0 || u >= n || v < 0 || v >= n) continue;
        edges.push_back({u,v});
        adj[u].push_back({v, i});
        if (u != v) adj[v].push_back({u, i});
    }

    vector<int> disc(n, -1), low(n, -1), parent(n, -1);
    vector<char> visited(n, false);
    int timer = 0;
    vector<pair<int,int>> bridges;

    function<void(int)> dfs = [&](int u){
        visited[u] = true;
        disc[u] = low[u] = timer++;
        for (auto &p : adj[u]){
            int v = p.first;
            int eid = p.second;
            if (!visited[v]){
                parent[v] = u;
                dfs(v);
                low[u] = min(low[u], low[v]);
                if (low[v] > disc[u]){
                    // (u,v) is a bridge
                    bridges.push_back({u,v});
                }
            } else if (v != parent[u]){
                // back-edge
                low[u] = min(low[u], disc[v]);
            }
        }
    };

    for (int i = 0; i < n; ++i)
        if (!visited[i]) dfs(i);

    cout << "Bridges found: " << bridges.size() << "\n";
    for (auto &e : bridges){
        cout << e.first << "," << e.second << "\n";
    }
    return 0;
}
