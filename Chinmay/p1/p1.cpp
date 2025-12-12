#include <bits/stdc++.h>
using namespace std;

// Extended Dijkstra's algorithm utility
// - Robust CSV parsing
// - Command-line options: --input <file>, --output <file>, --generate-sample
// - Validates node indices and non-negative weights
// - Measures runtime
// - Pretty-prints path and statistics
// - Falls back to writing a sample CSV when requested

// CSV format expected (header first line):
// num_nodes,num_edges,start,end
// then m lines: u,v,weight
// nodes are 0-indexed integers in [0, num_nodes-1]

static const long long INF = (1LL<<60);

struct Edge { int to; long long w; };

// Trim helpers
static inline void ltrim(string &s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch) { return !isspace(ch); }));
}
static inline void rtrim(string &s) {
    s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !isspace(ch); }).base(), s.end());
}
static inline void trim(string &s) { ltrim(s); rtrim(s); }

// Split CSV line into tokens using comma as separator, but be robust to spaces
vector<string> split_csv_line(const string &line) {
    vector<string> tokens;
    string cur;
    for (size_t i = 0; i < line.size(); ++i) {
        char ch = line[i];
        if (ch == ',') {
            string t = cur;
            trim(t);
            tokens.push_back(t);
            cur.clear();
        } else {
            cur.push_back(ch);
        }
    }
    string t = cur; trim(t);
    if (!t.empty() || line.size() && line.back() == ',') tokens.push_back(t);
    return tokens;
}

// Try parse integer, returns optional
optional<long long> parse_int_safe(const string &s) {
    if (s.empty()) return nullopt;
    char *endptr = nullptr;
    errno = 0;
    long long val = strtoll(s.c_str(), &endptr, 10);
    if (errno != 0) return nullopt;
    // ensure fully consumed (allow leading + or -)
    while (*endptr) {
        if (!isspace((unsigned char)*endptr)) return nullopt;
        ++endptr;
    }
    return val;
}

// Write a sample CSV file with simple graph
bool write_sample_csv(const string &filename) {
    ofstream ofs(filename);
    if (!ofs) return false;
    // small sample graph: 6 nodes, 8 edges
    ofs << "num_nodes,num_edges,start,end\n";
    ofs << "6,8,0,5\n";
    // edges: u,v,w
    ofs << "0,1,7\n";
    ofs << "0,2,9\n";
    ofs << "0,5,14\n";
    ofs << "1,2,10\n";
    ofs << "1,3,15\n";
    ofs << "2,3,11\n";
    ofs << "2,5,2\n";
    ofs << "3,4,6\n";
    ofs.close();
    return true;
}

int main(int argc, char **argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string input_filename;
    string output_filename;
    bool generate_sample = false;
    bool verbose = true;

    // Simple argument parsing
    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if (a == "--input" && i+1 < argc) { input_filename = argv[++i]; }
        else if (a == "--output" && i+1 < argc) { output_filename = argv[++i]; }
        else if (a == "--generate-sample") { generate_sample = true; }
        else if (a == "--quiet") { verbose = false; }
        else {
            cerr << "Unknown argument: " << a << "\n";
            cerr << "Usage: " << argv[0] << " [--input file.csv] [--output path.txt] [--generate-sample] [--quiet]\n";
            return 1;
        }
    }

    if (generate_sample) {
        const string sample_name = input_filename.empty() ? "sample_graph.csv" : input_filename;
        if (write_sample_csv(sample_name)) {
            cout << "Wrote sample CSV to: " << sample_name << "\n";
            if (input_filename.empty()) cout << "Use --input " << sample_name << " to run the solver on it.\n";
            return 0;
        } else {
            cerr << "Failed to write sample CSV to: " << sample_name << "\n";
            return 2;
        }
    }

    // If input filename specified, open it; otherwise read from stdin
    istream *inptr = &cin;
    ifstream ifs;
    if (!input_filename.empty()) {
        ifs.open(input_filename);
        if (!ifs) {
            cerr << "Failed to open input file: " << input_filename << "\n";
            return 3;
        }
        inptr = &ifs;
    }

    string header;
    if (!getline(*inptr, header)) {
        cerr << "No input received (empty stream).\n";
        cerr << "If you want a template CSV, run with --generate-sample.\n";
        return 4;
    }

    auto tokens = split_csv_line(header);
    if (tokens.size() < 4) {
        cerr << "Header parsing failed. Expected: num_nodes,num_edges,start,end\n";
        cerr << "Found tokens: " << tokens.size() << "\n";
        return 5;
    }

    auto n_opt = parse_int_safe(tokens[0]);
    auto m_opt = parse_int_safe(tokens[1]);
    auto s_opt = parse_int_safe(tokens[2]);
    auto t_opt = parse_int_safe(tokens[3]);
    if (!n_opt || !m_opt || !s_opt || !t_opt) {
        cerr << "Header contains invalid integer(s).\n";
        return 6;
    }

    int n = (int)*n_opt;
    int m = (int)*m_opt;
    int start = (int)*s_opt;
    int target = (int)*t_opt;

    if (n <= 0) { cerr << "Number of nodes must be positive.\n"; return 7; }
    if (m < 0) { cerr << "Number of edges cannot be negative.\n"; return 8; }
    if (start < 0 || start >= n) { cerr << "Start node out of range.\n"; return 9; }
    if (target < 0 || target >= n) { cerr << "Target node out of range.\n"; return 10; }

    vector<vector<Edge>> adj;
    adj.assign(n, {});

    int read_edges = 0;
    string line;

    // Read edges lines robustly; allow blank lines and comments prefixed by '#'
    while (read_edges < m && getline(*inptr, line)) {
        trim(line);
        if (line.empty()) continue;
        if (!line.empty() && line[0] == '#') continue; // comment

        auto parts = split_csv_line(line);
        if (parts.size() < 3) {
            if (verbose) cerr << "Skipping invalid edge line: '" << line << "'\n";
            continue;
        }

        auto u_opt = parse_int_safe(parts[0]);
        auto v_opt = parse_int_safe(parts[1]);
        auto w_opt = parse_int_safe(parts[2]);
        if (!u_opt || !v_opt || !w_opt) {
            if (verbose) cerr << "Skipping line with non-integer entries: '" << line << "'\n";
            continue;
        }
        int u = (int)*u_opt;
        int v = (int)*v_opt;
        long long w = *w_opt;
        if (u < 0 || u >= n || v < 0 || v >= n) {
            if (verbose) cerr << "Skipping out-of-range edge: " << u << "->" << v << "\n";
            continue;
        }
        if (w < 0) {
            if (verbose) cerr << "Skipping negative-weight edge: " << u << "->" << v << " weight=" << w << "\n";
            continue;
        }

        adj[u].push_back({v, w});
        ++read_edges;
    }

    if (read_edges < m) {
        if (verbose) cerr << "Warning: expected " << m << " edges but read " << read_edges << ". Proceeding with what we have.\n";
    }

    // Dijkstra's algorithm
    vector<long long> dist(n, INF);
    vector<int> parent(n, -1);
    vector<char> seen(n, 0);

    using pli = pair<long long,int>;
    priority_queue<pli, vector<pli>, greater<pli>> pq;

    dist[start] = 0;
    pq.push({0, start});

    auto t0 = chrono::high_resolution_clock::now();
    long long relaxations = 0;
    while (!pq.empty()) {
        auto [d,u] = pq.top(); pq.pop();
        if (d != dist[u]) continue;
        if (u == target) break; // early exit when we popped target with final dist
        if (seen[u]) continue;
        seen[u] = 1;
        for (const auto &e : adj[u]) {
            int v = e.to;
            long long w = e.w;
            ++relaxations;
            if (dist[v] > dist[u] + w) {
                dist[v] = dist[u] + w;
                parent[v] = u;
                pq.push({dist[v], v});
            }
        }
    }
    auto t1 = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = t1 - t0;

    if (dist[target] == INF) {
        cout << "unreachable\n";
        return 0;
    }

    // Reconstruct path
    vector<int> path;
    for (int cur = target; cur != -1; cur = parent[cur]) path.push_back(cur);
    reverse(path.begin(), path.end());

    // Output
    ostringstream outbuf;
    outbuf << "Total time: " << dist[target] << "\n";
    outbuf << "Path (size " << path.size() << "): ";
    for (size_t i = 0; i < path.size(); ++i) {
        if (i) outbuf << " -> ";
        outbuf << path[i];
    }
    outbuf << "\n";
    outbuf << "Nodes: " << n << ", edges read: " << read_edges << "\n";
    outbuf << "Relaxations performed: " << relaxations << "\n";
    outbuf << "Elapsed time (seconds): " << fixed << setprecision(6) << elapsed.count() << "\n";

    // Optionally write to output file
    if (!output_filename.empty()) {
        ofstream ofs_out(output_filename);
        if (!ofs_out) {
            cerr << "Failed to open output file: " << output_filename << "\n";
            return 11;
        }
        ofs_out << outbuf.str();
        ofs_out.close();
        if (verbose) cout << "Wrote result to: " << output_filename << "\n";
    } else {
        cout << outbuf.str();
    }

    return 0;
}

/*
SAMPLE CSV (suitable changes):

The program expects a header line with four comma-separated integers: num_nodes,num_edges,start,end
Followed by exactly num_edges lines giving edges in the form: u,v,weight
Nodes are 0-indexed.

Example CSV that matches the small default sample written by --generate-sample:

num_nodes,num_edges,start,end
6,8,0,5
0,1,7
0,2,9
0,5,14
1,2,10
1,3,15
2,3,11
2,5,2
3,4,6

Notes on "suitable changes":
- Ensure the header uses the same number of edges as actual edge lines, or the program will warn but continue.
- Remove any negative weights (Dijkstra requires non-negative weights).
- Add optional blank lines or comment lines starting with '#' which the parser will ignore.
- If you have a very large CSV (for example the uploaded file with thousands of edges), you can run:
    ./dijkstra_extended --input ev_graph_1000_edges.csv --output shortest_path.txt
  and it will parse and run. If you don't have the file, create one using:
    ./dijkstra_extended --generate-sample --input my_sample.csv

*/
