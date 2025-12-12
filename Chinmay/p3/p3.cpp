#include <bits/stdc++.h>
using namespace std;

// Connectivity checker for an undirected graph (subway network).
// Input CSV format (first line header):
// num_nodes,num_edges,hub
// Then num_edges lines: u,v
// Nodes are 0-indexed integers in [0, num_nodes-1].

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string header;
    if (!getline(cin, header)) return 0;
    stringstream ss(header);
    int n, m, hub;
    char comma;
    ss >> n >> comma >> m >> comma >> hub;

    vector<vector<int>> adj(n);
    for (int i = 0; i < m; ++i) {
        string line;
        if (!getline(cin, line)) break;
        if (line.size() == 0) { --i; continue; }
        stringstream es(line);
        int u, v;
        char c;
        es >> u >> c >> v;
        if (u < 0 || u >= n || v < 0 || v >= n) continue;
        // undirected subway edges
        adj[u].push_back(v);
        if (u != v) adj[v].push_back(u);
    }

    vector<char> visited(n, false);
    queue<int> q;
    visited[hub] = true;
    q.push(hub);
    int visited_count = 0;

    while (!q.empty()) {
        int u = q.front(); q.pop();
        ++visited_count;
        for (int v : adj[u]) {
            if (!visited[v]) {
                visited[v] = true;
                q.push(v);
            }
        }
    }

    if (visited_count == n) {
        cout << "CONNECTED\n";
        cout << "All " << n << " stations reachable from hub " << hub << "\n";
    } else {
        cout << "DISCONNECTED\n";
        cout << visited_count << " of " << n << " stations are reachable from hub " << hub << "\n";
        cout << "Unreachable stations (indices):\n";
        for (int i = 0; i < n; ++i) if (!visited[i]) cout << i << (i+1==n? '\n' : ' ');
        cout << "\n";
    }

    return 0;
}
