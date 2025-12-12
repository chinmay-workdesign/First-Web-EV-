#include <bits/stdc++.h>
using namespace std;

// Extended Connectivity Checker for undirected graphs (e.g., subway network)
// Features added:
// - Robust CSV parsing with comments and blank-line tolerance
// - Command-line options: --input <file>, --output <file>, --generate-sample, --method <bfs|dfs>, --quiet
// - Detects connected components, sizes, and lists nodes in each component
// - Identifies articulation points and bridges (critical stations/links)
// - Optionally prints unreachable node indices and component statistics
// - Measures runtime and prints a compact report
//
// Expected CSV format (header first line):
// num_nodes,num_edges,hub
// then m lines: u,v
// nodes are 0-indexed integers in [0, num_nodes-1]
//
// Notes on "suitable changes" to CSV:
// - header's num_edges should match count of (non-empty, non-comment) edge lines (program warns otherwise)
// - allow comments (lines starting with '#') and blank lines
// - self-loops are allowed but treated specially (counted but ignored for connectivity when necessary)
// - ensure indices are within [0,n-1]

// Small helpers
static inline void ltrim(string &s) { s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch){ return !isspace(ch); })); }
static inline void rtrim(string &s) { s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch){ return !isspace(ch); }).base(), s.end()); }
static inline void trim(string &s) { ltrim(s); rtrim(s); }

vector<string> split_csv_line(const string &line) {
    vector<string> tokens;
    string cur;
    for (size_t i = 0; i < line.size(); ++i) {
        char ch = line[i];
        if (ch == ',') { string t = cur; trim(t); tokens.push_back(t); cur.clear(); }
        else cur.push_back(ch);
    }
    string t = cur; trim(t);
    if (!t.empty() || (line.size() && line.back() == ',')) tokens.push_back(t);
    return tokens;
}

optional<long long> parse_int_safe(const string &s) {
    if (s.empty()) return nullopt;
    char *endptr = nullptr;
    errno = 0;
    long long val = strtoll(s.c_str(), &endptr, 10);
    if (errno != 0) return nullopt;
    while (*endptr) { if (!isspace((unsigned char)*endptr)) return nullopt; ++endptr; }
    return val;
}

bool write_sample_csv(const string &filename) {
    ofstream ofs(filename);
    if (!ofs) return false;
    // simple sample: two components and some articulation points
    ofs << "num_nodes,num_edges,hub\n";
    ofs << "10,9,0\n"; // 10 nodes, 9 edges, hub = 0
    ofs << "0,1\n";
    ofs << "1,2\n";
    ofs << "2,3\n";
    ofs << "3,4\n"; // chain 0-1-2-3-4
    ofs << "2,5\n"; // branch from 2 to 5
    ofs << "6,7\n";
    ofs << "7,8\n";
    ofs << "8,6\n"; // small triangle 6-7-8
    ofs << "9,9\n"; // self-loop node 9 isolated
    ofs.close();
    return true;
}

// Tarjan's algorithm for articulation points and bridges
struct GraphAnalysis {
    int n;
    vector<vector<int>> adj;
    GraphAnalysis(int n_ = 0) { n = n_; adj.assign(n, {}); }
    void add_edge(int u, int v) {
        if (u < 0 || u >= n || v < 0 || v >= n) return;
        adj[u].push_back(v);
        if (u != v) adj[v].push_back(u); // undirected; avoid duplicate for self-loop
    }

    // returns components vector where comp[u] = component id, and list of components as vectors
    pair<vector<int>, vector<vector<int>>> components() {
        vector<int> comp(n, -1);
        vector<vector<int>> parts;
        int cid = 0;
        for (int i = 0; i < n; ++i) {
            if (comp[i] != -1) continue;
            // BFS for component
            queue<int> q; q.push(i); comp[i] = cid; parts.emplace_back();
            while (!q.empty()) {
                int u = q.front(); q.pop();
                parts.back().push_back(u);
                for (int v : adj[u]) if (comp[v] == -1) {
                    comp[v] = cid; q.push(v);
                }
            }
            ++cid;
        }
        return {comp, parts};
    }

    // Tarjan to find articulation points and bridges
    void tarjan_articulation_and_bridges(vector<char> &is_art, vector<pair<int,int>> &bridges) {
        is_art.assign(n, 0);
        bridges.clear();
        vector<int> disc(n, -1), low(n, -1), parent(n, -1);
        int time = 0;
        function<void(int)> dfs = [&](int u) {
            disc[u] = low[u] = time++;
            int children = 0;
            for (int v : adj[u]) {
                if (disc[v] == -1) {
                    ++children;
                    parent[v] = u;
                    dfs(v);
                    low[u] = min(low[u], low[v]);
                    // articulation
                    if (parent[u] == -1 && children > 1) is_art[u] = 1;
                    if (parent[u] != -1 && low[v] >= disc[u]) is_art[u] = 1;
                    // bridge
                    if (low[v] > disc[u]) bridges.emplace_back(u, v);
                } else if (v != parent[u]) {
                    low[u] = min(low[u], disc[v]);
                }
            }
        };
        for (int i = 0; i < n; ++i) if (disc[i] == -1) dfs(i);
    }
};

int main(int argc, char **argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string input_filename;
    string output_filename;
    bool generate_sample = false;
    bool verbose = true;
    string method = "bfs"; // or dfs

    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if (a == "--input" && i+1 < argc) input_filename = argv[++i];
        else if (a == "--output" && i+1 < argc) output_filename = argv[++i];
        else if (a == "--generate-sample") generate_sample = true;
        else if (a == "--quiet") verbose = false;
        else if (a == "--method" && i+1 < argc) method = argv[++i];
        else { cerr << "Unknown arg: " << a << "\n"; cerr << "Usage: " << argv[0] << " [--input file.csv] [--output file.txt] [--generate-sample] [--method bfs|dfs] [--quiet]\n"; return 1; }
    }

    if (generate_sample) {
        const string sample_name = input_filename.empty() ? "sample_subway.csv" : input_filename;
        if (write_sample_csv(sample_name)) {
            cout << "Wrote sample CSV to: " << sample_name << "\n";
            if (input_filename.empty()) cout << "Use --input " << sample_name << " to run the checker on it.\n";
            return 0;
        } else { cerr << "Failed to write sample CSV to: " << sample_name << "\n"; return 2; }
    }

    istream *inptr = &cin;
    ifstream ifs;
    if (!input_filename.empty()) {
        ifs.open(input_filename);
        if (!ifs) { cerr << "Failed to open input file: " << input_filename << "\n"; return 3; }
        inptr = &ifs;
    }

    string header;
    if (!getline(*inptr, header)) { cerr << "No input received.\n"; return 4; }
    auto header_tokens = split_csv_line(header);
    if (header_tokens.size() < 3) { cerr << "Header parse failed. Expected: num_nodes,num_edges,hub\n"; return 5; }
    auto n_opt = parse_int_safe(header_tokens[0]);
    auto m_opt = parse_int_safe(header_tokens[1]);
    auto hub_opt = parse_int_safe(header_tokens[2]);
    if (!n_opt || !m_opt || !hub_opt) { cerr << "Header contains invalid integer(s).\n"; return 6; }
    int n = (int)*n_opt; int m = (int)*m_opt; int hub = (int)*hub_opt;
    if (n <= 0) { cerr << "Number of nodes must be positive.\n"; return 7; }
    if (m < 0) { cerr << "Number of edges cannot be negative.\n"; return 8; }
    if (hub < 0 || hub >= n) { cerr << "Hub index out of range.\n"; return 9; }

    GraphAnalysis graph(n);
    int read_edges = 0;
    string line;
    vector<pair<int,int>> raw_edges;

    while (read_edges < m && getline(*inptr, line)) {
        trim(line);
        if (line.empty()) continue;
        if (line[0] == '#') continue;
        auto parts = split_csv_line(line);
        if (parts.size() < 2) { if (verbose) cerr << "Skipping invalid edge line: '" << line << "'\n"; continue; }
        auto u_opt = parse_int_safe(parts[0]);
        auto v_opt = parse_int_safe(parts[1]);
        if (!u_opt || !v_opt) { if (verbose) cerr << "Skipping non-integer line: '" << line << "'\n"; continue; }
        int u = (int)*u_opt; int v = (int)*v_opt;
        if (u < 0 || u >= n || v < 0 || v >= n) { if (verbose) cerr << "Skipping out-of-range edge: " << u << "->" << v << "\n"; continue; }
        graph.add_edge(u, v);
        raw_edges.emplace_back(u, v);
        ++read_edges;
    }
    if (read_edges < m) if (verbose) cerr << "Warning: expected " << m << " edges but read " << read_edges << ". Proceeding.\n";

    // Measure runtime for connectivity analysis
    auto t0 = chrono::high_resolution_clock::now();

    // Run BFS or DFS from hub
    vector<char> visited(n, false);
    int visited_count = 0;
    if (method == "bfs") {
        queue<int> q; visited[hub] = true; q.push(hub);
        while (!q.empty()) { int u = q.front(); q.pop(); ++visited_count; for (int v : graph.adj[u]) if (!visited[v]) { visited[v] = true; q.push(v); } }
    } else {
        // DFS iterative
        vector<int> stack; stack.push_back(hub); visited[hub] = true;
        while (!stack.empty()) { int u = stack.back(); stack.pop_back(); ++visited_count; for (int v : graph.adj[u]) if (!visited[v]) { visited[v] = true; stack.push_back(v); } }
    }

    // Components and articulation points / bridges
    auto comp_pair = graph.components();
    vector<int> comp = comp_pair.first;
    vector<vector<int>> components = comp_pair.second;

    vector<char> is_art;
    vector<pair<int,int>> bridges;
    graph.tarjan_articulation_and_bridges(is_art, bridges);

    // List unreachable nodes
    vector<int> unreachable_nodes;
    for (int i = 0; i < n; ++i) if (!visited[i]) unreachable_nodes.push_back(i);

    auto t1 = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = t1 - t0;

    // Prepare output
    ostringstream out;
    out << "Connectivity Report\n";
    out << "Nodes: " << n << ", edges (declared): " << m << ", edges (read): " << read_edges << "\n";
    out << "Method: " << method << "\n";
    out << "Hub: " << hub << "\n";
    out << "Visited from hub: " << visited_count << "\n";
    out << (visited_count == n ? "CONNECTED\n" : "DISCONNECTED\n");
    out << "Elapsed (s): " << fixed << setprecision(6) << elapsed.count() << "\n";
    out << "Number of components: " << components.size() << "\n";
    out << "Component sizes:\n";
    for (size_t i = 0; i < components.size(); ++i) {
        out << "  C" << i << ": size=" << components[i].size() << " nodes\n";
    }

    out << "\nArticulation points (critical stations):\n";
    for (int i = 0; i < n; ++i) if (is_art[i]) out << i << " ";
    out << "\n\nBridges (critical links u-v):\n";
    for (auto &e : bridges) out << e.first << "-" << e.second << "\n";

    out << "\nUnreachable stations from hub (if any):\n";
    if (unreachable_nodes.empty()) out << "  None\n";
    else {
        for (int x : unreachable_nodes) out << x << " ";
        out << "\n";
    }

    out << "\nSample of raw edges (first 20):\n";
    for (size_t i = 0; i < raw_edges.size() && i < 20; ++i) out << raw_edges[i].first << "," << raw_edges[i].second << "\n";

    // Optionally write to a file
    if (!output_filename.empty()) {
        ofstream ofs(output_filename);
        if (!ofs) { cerr << "Failed to open output file: " << output_filename << "\n"; return 11; }
        ofs << out.str(); ofs.close(); if (verbose) cout << "Wrote report to: " << output_filename << "\n";
    } else {
        cout << out.str();
    }

    return 0;
}

/*
SAMPLE CSV and "suitable changes":

The program expects a header: num_nodes,num_edges,hub
Then m lines of edges: u,v

Example sample produced by --generate-sample (already included above):

num_nodes,num_edges,hub
10,9,0
0,1
1,2
2,3
3,4
2,5
6,7
7,8
8,6
9,9
*/
