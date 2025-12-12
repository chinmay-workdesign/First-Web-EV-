// all_pairs_paths.cpp
// Compute shortest paths between ALL pairs of districts and shelters.
// - Reads input CSV "graph_fw.csv" with rows:
//   NODE,node_id,role      (role: D for district, S for shelter, O other)
//   E,node_u,node_v,weight (weight = non-negative travel time minutes)
// - If V <= FW_THRESHOLD, runs Floyd-Warshall (dense DP).
// - Otherwise runs Dijkstra from each node in set (districts U shelters).
// - Outputs distances_pairs.csv with source,target,distance_minutes and path files.
//
// Compile: g++ -std=c++17 -O2 -o all_pairs_paths all_pairs_paths.cpp
//
// Usage: ./all_pairs_paths graph_fw.csv
//
// Notes:
// - The program auto-detects input format used in the generated CSV.
// - For very large V, consider running Johnson's algorithm or further optimizations.

#include <bits/stdc++.h>
using namespace std;
using ll = long long;
const ll INFLL = (ll)4e18;

// -------------------------- CSV parsing utilities -------------------------

static inline void trim(string &s) {
    const char* ws = " \t\r\n";
    size_t a = s.find_first_not_of(ws);
    if (a == string::npos) { s.clear(); return; }
    size_t b = s.find_last_not_of(ws);
    s = s.substr(a, b - a + 1);
}

bool split_csv_line(const string &line, vector<string> &out) {
    out.clear();
    string cur;
    bool in_quotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') { in_quotes = !in_quotes; continue; }
        if (c == ',' && !in_quotes) { out.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    out.push_back(cur);
    for (auto &s : out) trim(s);
    return true;
}

// ------------------------ Graph data structures ---------------------------

struct Edge { int to; ll w; };
struct Graph {
    int V = 0;
    vector<vector<Edge>> adj;
    Graph() {}
    Graph(int n): V(n), adj(n) {}
    void add_edge(int u, int v, ll w) {
        adj[u].push_back({v, w});
    }
};

// -------------------------- Floyd-Warshall impl ---------------------------

void floyd_warshall_with_next(const vector<vector<ll>> &init_dist, vector<vector<ll>> &dist, vector<vector<int>> &next) {
    int n = (int)init_dist.size();
    dist = init_dist;
    next.assign(n, vector<int>(n, -1));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (dist[i][j] < INFLL) next[i][j] = j;
        }
    }
    for (int k = 0; k < n; ++k) {
        for (int i = 0; i < n; ++i) {
            if (dist[i][k] == INFLL) continue;
            for (int j = 0; j < n; ++j) {
                if (dist[k][j] == INFLL) continue;
                ll nd = dist[i][k] + dist[k][j];
                if (nd < dist[i][j]) {
                    dist[i][j] = nd;
                    next[i][j] = next[i][k];
                }
            }
        }
    }
}

// Reconstruct path from u to v using next matrix (returns empty if unreachable)
vector<int> reconstruct_path_fw(int u, int v, const vector<vector<int>> &next) {
    if (next[u][v] == -1) return {};
    vector<int> path;
    int cur = u;
    while (cur != v) {
        path.push_back(cur);
        cur = next[cur][v];
        if (cur == -1) return {}; // safety
    }
    path.push_back(v);
    return path;
}

// ------------------------- Dijkstra implementation ------------------------

void dijkstra_single_source(const Graph &G, int src, vector<ll> &dist, vector<int> &parent) {
    int n = G.V;
    dist.assign(n, INFLL);
    parent.assign(n, -1);
    dist[src] = 0;
    using pli = pair<ll,int>;
    priority_queue<pli, vector<pli>, greater<pli>> pq;
    pq.push({0, src});
    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (d != dist[u]) continue;
        for (auto &e : G.adj[u]) {
            int v = e.to; ll nd = d + e.w;
            if (nd < dist[v]) {
                dist[v] = nd;
                parent[v] = u;
                pq.push({nd, v});
            }
        }
    }
}

// reconstruct path from parent array (parent[v] = previous node to reach v)
vector<int> reconstruct_path_from_parent(int src, int v, const vector<int> &parent) {
    if (parent[v] == -1 && v != src) return {};
    vector<int> rev;
    int cur = v;
    while (cur != -1) {
        rev.push_back(cur);
        if (cur == src) break;
        cur = parent[cur];
    }
    reverse(rev.begin(), rev.end());
    return rev;
}

// --------------------------- Main program logic ---------------------------

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " graph_fw.csv [--fw-threshold N] [--outprefix prefix]\n";
        return 1;
    }
    string infile = argv[1];
    int FW_THRESHOLD = 500; // default threshold for running Floyd-Warshall
    string outprefix = "allpairs";
    for (int i = 2; i < argc; ++i) {
        string s = argv[i];
        if (s == "--fw-threshold" && i+1 < argc) { FW_THRESHOLD = stoi(argv[++i]); }
        else if (s == "--outprefix" && i+1 < argc) { outprefix = argv[++i]; }
    }

    // Read CSV
    ifstream fin(infile);
    if (!fin.is_open()) {
        cerr << "Cannot open " << infile << "\n"; return 1;
    }
    string header;
    getline(fin, header);
    string line;
    vector<tuple<string,string,string>> rows; // type,u,v_or_role
    while (getline(fin, line)) {
        if (line.empty()) continue;
        vector<string> cols;
        split_csv_line(line, cols);
        if (cols.size() < 3) continue;
        string type = cols[0], u = cols[1], v = cols[2];
        string weight = (cols.size() >= 4 ? cols[3] : "");
        trim(type); trim(u); trim(v); trim(weight);
        if (type == "E") {
            rows.emplace_back("E", u + "|" + v, weight); // store combined edge
        } else if (type == "NODE") {
            rows.emplace_back("NODE", u, v); // node with role in v
        } else {
            // unknown rows ignored
        }
    }
    fin.close();

    // Build node index
    unordered_map<string,int> node_index;
    vector<string> node_names;
    vector<char> node_role; // 'D' district, 'S' shelter, 'O' other
    // First pass: collect nodes from NODE rows to preserve labeling
    for (auto &t : rows) {
        string type = get<0>(t);
        if (type == "NODE") {
            string u = get<1>(t);
            string role = get<2>(t);
            if (node_index.find(u) == node_index.end()) {
                int idx = (int)node_names.size();
                node_index[u] = idx;
                node_names.push_back(u);
                char r = 'O';
                if (role == "D") r = 'D';
                else if (role == "S") r = 'S';
                node_role.push_back(r);
            }
        }
    }
    // In case edges reference nodes not present as NODE rows, add them
    for (auto &t : rows) {
        if (get<0>(t) != "E") continue;
        string uv = get<1>(t);
        auto pos = uv.find('|');
        string u = uv.substr(0, pos), v = uv.substr(pos+1);
        if (node_index.find(u) == node_index.end()) {
            int idx = (int)node_names.size();
            node_index[u] = idx;
            node_names.push_back(u); node_role.push_back('O');
        }
        if (node_index.find(v) == node_index.end()) {
            int idx = (int)node_names.size();
            node_index[v] = idx;
            node_names.push_back(v); node_role.push_back('O');
        }
    }
    int V = (int)node_names.size();
    cout << "Total nodes detected: " << V << "\n";

    // Build adjacency list and initial distance matrix if needed
    vector<vector<pair<int,ll>>> adj(V);
    // For FW we will initialize dist matrix after parsing edges
    vector<vector<ll>> distmat;
    if (V <= FW_THRESHOLD) {
        distmat.assign(V, vector<ll>(V, INFLL));
        for (int i = 0; i < V; ++i) distmat[i][i] = 0;
    }
    // Parse edges and fill adj
    for (auto &t : rows) {
        if (get<0>(t) != "E") continue;
        string uv = get<1>(t);
        auto pos = uv.find('|');
        string su = uv.substr(0, pos), sv = uv.substr(pos+1);
        string wstr = get<2>(t);
        ll w = 1;
        try { if (!wstr.empty()) w = stoll(wstr); } catch(...) { w = 1; }
        int u = node_index[su], v = node_index[sv];
        // assume undirected travel times; if directed treat accordingly
        adj[u].push_back({v, w});
        adj[v].push_back({u, w});
        if (V <= FW_THRESHOLD) {
            if (w < distmat[u][v]) {
                distmat[u][v] = w;
            }
            if (w < distmat[v][u]) {
                distmat[v][u] = w;
            }
        }
    }

    // Collect indices of districts and shelters
    vector<int> districts, shelters, nodes_of_interest;
    for (int i = 0; i < V; ++i) {
        if (node_role[i] == 'D') { districts.push_back(i); nodes_of_interest.push_back(i); }
        if (node_role[i] == 'S') { shelters.push_back(i); nodes_of_interest.push_back(i); }
    }
    // Unique nodes_of_interest (districts U shelters) -- already unique
    cout << "Districts: " << districts.size() << ", Shelters: " << shelters.size() << "\n";

    // Decide method
    bool use_fw = (V <= FW_THRESHOLD);
    if (use_fw) cout << "V <= " << FW_THRESHOLD << " -> using Floyd-Warshall (O(V^3)).\n";
    else cout << "V > " << FW_THRESHOLD << " -> using repeated Dijkstra from each district/shelter.\n";

    // Outputs
    string out_pairs = outprefix + "_distances_pairs.csv";
    ofstream fout(out_pairs);
    fout << "source_id,source_role,target_id,target_role,distance_minutes\n";

    if (use_fw) {
        // Run Floyd-Warshall with next matrix for path reconstruction
        vector<vector<ll>> dist;
        vector<vector<int>> next;
        floyd_warshall_with_next(distmat, dist, next);
        // Write distances between all pairs of districts and shelters (cartesian product)
        for (int si : nodes_of_interest) {
            for (int ti : nodes_of_interest) {
                ll d = dist[si][ti];
                string ds = (d >= INFLL/4) ? string("INF") : to_string(d);
                fout << node_names[si] << "," << node_role[si] << "," << node_names[ti] << "," << node_role[ti] << "," << ds << "\n";
                // Optionally write path file if reachable
                if (d < INFLL/4) {
                    vector<int> path = reconstruct_path_fw(si, ti, next);
                    // write path to file
                    string pathfile = outprefix + "_path_" + node_names[si] + "_to_" + node_names[ti] + ".txt";
                    ofstream pf(pathfile);
                    for (size_t k = 0; k < path.size(); ++k) {
                        pf << node_names[path[k]];
                        if (k+1 < path.size()) pf << " -> ";
                    }
                    pf << "\n";
                    pf.close();
                }
            }
        }
    } else {
        // Use Dijkstra from each node of interest
        Graph graph;
        graph.V = V;
        graph.adj.resize(V);
        for (int u = 0; u < V; ++u) {
            for (auto &e : adj[u]) graph.adj[u].push_back({e.first, e.second});
        }
        // For each source in nodes_of_interest run Dijkstra
        for (int sidx = 0; sidx < (int)nodes_of_interest.size(); ++sidx) {
            int src = nodes_of_interest[sidx];
            vector<ll> dist;
            vector<int> parent;
            dijkstra_single_source(graph, src, dist, parent);
            for (int t : nodes_of_interest) {
                ll d = dist[t];
                string ds = (d >= INFLL/4) ? string("INF") : to_string(d);
                fout << node_names[src] << "," << node_role[src] << "," << node_names[t] << "," << node_role[t] << "," << ds << "\n";
                // optional path write
                if (d < INFLL/4) {
                    vector<int> path = reconstruct_path_from_parent(src, t, parent);
                    string pathfile = outprefix + "_path_" + node_names[src] + "_to_" + node_names[t] + ".txt";
                    ofstream pf(pathfile);
                    for (size_t k = 0; k < path.size(); ++k) {
                        pf << node_names[path[k]];
                        if (k+1 < path.size()) pf << " -> ";
                    }
                    pf << "\n";
                    pf.close();
                }
            }
            // optional progress
            if ((sidx+1) % 10 == 0) cerr << "Completed Dijkstra for " << (sidx+1) << " / " << nodes_of_interest.size() << " sources\n";
        }
    }
    fout.close();
    cout << "Wrote pairwise distances to " << out_pairs << "\n";
    cout << "Done.\n";
    return 0;
}
