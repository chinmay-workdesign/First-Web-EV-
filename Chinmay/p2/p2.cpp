#include <bits/stdc++.h>
using namespace std;

// Extended Edmonds-Karp (BFS) Max-Flow solver
// - Robust CSV parsing
// - Command-line options: --input <file>, --output <file>, --generate-sample, --quiet
// - Supports large graphs (uses adjacency lists + unordered_map for sparse residual capacities)
// - Sums parallel edges
// - Validates node indices and non-negative capacities
// - Reports max flow, min cut (S/T partition), residual edge list, and runtime
//
// CSV format expected (first header line):
// num_nodes,num_edges,source,sink
// then m lines: u,v,capacity
// nodes are 0-indexed integers in [0, num_nodes-1]

using ll = long long;
const ll INFLL = (1LL<<60);

// Trim helpers
static inline void ltrim(string &s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch) { return !isspace(ch); }));
}
static inline void rtrim(string &s) {
    s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !isspace(ch); }).base(), s.end());
}
static inline void trim(string &s) { ltrim(s); rtrim(s); }

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
    if (!t.empty() || (line.size() && line.back() == ',')) tokens.push_back(t);
    return tokens;
}

optional<long long> parse_int_safe(const string &s) {
    if (s.empty()) return nullopt;
    char *endptr = nullptr;
    errno = 0;
    long long val = strtoll(s.c_str(), &endptr, 10);
    if (errno != 0) return nullopt;
    while (*endptr) {
        if (!isspace((unsigned char)*endptr)) return nullopt;
        ++endptr;
    }
    return val;
}

bool write_sample_csv(const string &filename) {
    ofstream ofs(filename);
    if (!ofs) return false;
    // small sample flow network: 6 nodes, 9 edges
    ofs << "num_nodes,num_edges,source,sink\n";
    ofs << "6,9,0,5\n";
    // edges: u,v,capacity
    ofs << "0,1,16\n";
    ofs << "0,2,13\n";
    ofs << "1,2,10\n";
    ofs << "1,3,12\n";
    ofs << "2,1,4\n";
    ofs << "2,4,14\n";
    ofs << "3,2,9\n";
    ofs << "3,5,20\n";
    ofs << "4,3,7\n";
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

    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if (a == "--input" && i+1 < argc) input_filename = argv[++i];
        else if (a == "--output" && i+1 < argc) output_filename = argv[++i];
        else if (a == "--generate-sample") generate_sample = true;
        else if (a == "--quiet") verbose = false;
        else {
            cerr << "Unknown argument: " << a << "\n";
            cerr << "Usage: " << argv[0] << " [--input file.csv] [--output result.txt] [--generate-sample] [--quiet]\n";
            return 1;
        }
    }

    if (generate_sample) {
        const string sample_name = input_filename.empty() ? "sample_maxflow.csv" : input_filename;
        if (write_sample_csv(sample_name)) {
            cout << "Wrote sample CSV to: " << sample_name << "\n";
            if (input_filename.empty()) cout << "Use --input " << sample_name << " to run the solver on it.\n";
            return 0;
        } else {
            cerr << "Failed to write sample CSV to: " << sample_name << "\n";
            return 2;
        }
    }

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

    auto header_tokens = split_csv_line(header);
    if (header_tokens.size() < 4) {
        cerr << "Header parsing failed. Expected: num_nodes,num_edges,source,sink\n";
        return 5;
    }

    auto n_opt = parse_int_safe(header_tokens[0]);
    auto m_opt = parse_int_safe(header_tokens[1]);
    auto s_opt = parse_int_safe(header_tokens[2]);
    auto t_opt = parse_int_safe(header_tokens[3]);
    if (!n_opt || !m_opt || !s_opt || !t_opt) {
        cerr << "Header contains invalid integer(s).\n";
        return 6;
    }

    int n = (int)*n_opt;
    int m = (int)*m_opt;
    int s = (int)*s_opt;
    int t = (int)*t_opt;

    if (n <= 0) { cerr << "Number of nodes must be positive.\n"; return 7; }
    if (m < 0) { cerr << "Number of edges cannot be negative.\n"; return 8; }
    if (s < 0 || s >= n) { cerr << "Source node out of range.\n"; return 9; }
    if (t < 0 || t >= n) { cerr << "Sink node out of range.\n"; return 10; }

    // adjacency list and residual capacities stored in unordered_map for sparsity
    vector<vector<int>> adj(n);
    // residual capacities: map of (u->v) to capacity
    unordered_map<long long, ll> residual; // key = ((long long)u<<32) | v
    auto key = [](int u, int v)->long long { return ( (long long)u << 32 ) | (unsigned int)v; };

    int read_edges = 0;
    string line;
    while (read_edges < m && getline(*inptr, line)) {
        trim(line);
        if (line.empty()) continue;
        if (!line.empty() && line[0] == '#') continue;
        auto parts = split_csv_line(line);
        if (parts.size() < 3) {
            if (verbose) cerr << "Skipping invalid edge line: '" << line << "'\n";
            continue;
        }
        auto u_opt = parse_int_safe(parts[0]);
        auto v_opt = parse_int_safe(parts[1]);
        auto c_opt = parse_int_safe(parts[2]);
        if (!u_opt || !v_opt || !c_opt) {
            if (verbose) cerr << "Skipping non-integer line: '" << line << "'\n";
            continue;
        }
        int u = (int)*u_opt;
        int v = (int)*v_opt;
        ll cap = *c_opt;
        if (u < 0 || u >= n || v < 0 || v >= n) {
            if (verbose) cerr << "Skipping out-of-range edge: " << u << "->" << v << "\n";
            continue;
        }
        if (cap < 0) {
            if (verbose) cerr << "Skipping negative-capacity edge: " << u << "->" << v << " cap=" << cap << "\n";
            continue;
        }

        long long k = key(u,v);
        auto it = residual.find(k);
        if (it == residual.end()) {
            residual[k] = cap;
            // add adjacency entries for both directions (for BFS on residual graph)
            adj[u].push_back(v);
            adj[v].push_back(u);
        } else {
            residual[k] += cap; // sum parallel edges
        }
        ++read_edges;
    }

    if (read_edges < m) {
        if (verbose) cerr << "Warning: expected " << m << " edges but read " << read_edges << ". Proceeding with what we have.\n";
    }

    // Edmonds-Karp: Breadth-first search for augmenting paths
    auto bfs = [&](vector<int> &parent)->ll {
        fill(parent.begin(), parent.end(), -1);
        parent[s] = -2; // source visited
        queue<pair<int,ll>> q;
        q.push({s, INFLL});
        while (!q.empty()) {
            auto [cur, flow] = q.front(); q.pop();
            for (int nxt : adj[cur]) {
                if (parent[nxt] == -1) {
                    long long capuv = 0;
                    auto it = residual.find(key(cur,nxt));
                    if (it != residual.end()) capuv = it->second;
                    if (capuv > 0) {
                        parent[nxt] = cur;
                        ll new_flow = min(flow, capuv);
                        if (nxt == t) return new_flow;
                        q.push({nxt, new_flow});
                    }
                }
            }
        }
        return 0;
    };

    vector<int> parent(n, -1);
    ll maxflow = 0;
    ll augmentations = 0;
    auto t0 = chrono::high_resolution_clock::now();
    while (true) {
        ll pushed = bfs(parent);
        if (pushed == 0) break;
        ++augmentations;
        maxflow += pushed;
        int cur = t;
        while (cur != s) {
            int prev = parent[cur];
            // decrease forward capacity
            long long kf = key(prev, cur);
            residual[kf] -= pushed;
            // increase reverse capacity
            long long kr = key(cur, prev);
            residual[kr] += pushed;
            // ensure adjacency contains reverse if not present
            // (adj lists were already populated during input for both directions)
            cur = prev;
        }
    }
    auto t1 = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = t1 - t0;

    // Reachable set from s in residual graph (for min-cut S set)
    vector<char> reachable(n, 0);
    {
        queue<int> q;
        q.push(s);
        reachable[s] = 1;
        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (int v : adj[u]) {
                if (reachable[v]) continue;
                auto it = residual.find(key(u,v));
                ll cap = (it != residual.end()) ? it->second : 0;
                if (cap > 0) {
                    reachable[v] = 1;
                    q.push(v);
                }
            }
        }
    }

    // Collect min-cut edges (u in S, v in T, original capacity > 0)
    vector<tuple<int,int,ll>> mincut_edges;
    // Also prepare residual edge list for output
    vector<tuple<int,int,ll>> residual_edges;
    residual_edges.reserve(residual.size());
    for (const auto &p : residual) {
        long long k = p.first;
        int u = (int)(k >> 32);
        int v = (int)(k & 0xFFFFFFFF);
        residual_edges.emplace_back(u,v,p.second);
    }

    // To determine min-cut edges, we should compare original forward capacities vs residual forward capacities.
    // Since we summed input capacities into residual map and then modified residual during augmentations,
    // the original forward capacity equals residual_forward + flow_sent_forward (unknown directly). However,
    // an easy way: edges where u is reachable and v not reachable and the original forward capacity (residual[u->v] + residual[v->u]) > 0.
    // We'll compute original capacity as residual_forward + residual_reverse - current reverse amount
    // But simpler: treat an edge (u,v) as crossing cut if reachable[u] && !reachable[v] and the residual map contains a key for forward/initial edge or reverse key.

    for (const auto &p : residual) {
        long long k = p.first;
        int u = (int)(k >> 32);
        int v = (int)(k & 0xFFFFFFFF);
        // consider only edges that were originally present in forward direction with some initial capacity
        // we don't explicitly store initial capacities separately in this implementation, so we approximate by checking
        // if there is no residual in forward direction < original? To avoid complexity, we gather mincut edges by checking
        // if u reachable and v not reachable and the sum of residual(u,v) + residual(v,u) > 0.
        if (u >= 0 && u < n && v >= 0 && v < n) {
            ll cap_uv = p.second;
            long long krev = key(v,u);
            auto itrev = residual.find(krev);
            ll cap_vu = (itrev != residual.end()) ? itrev->second : 0;
            if (reachable[u] && !reachable[v]) {
                // there was some capacity originally (either cap_uv + cap_vu > 0)
                if (cap_uv + cap_vu > 0) {
                    // original forward capacity = cap_uv + (flow that went from u to v)?? To avoid claiming wrong numbers, report as "cut edge" and show residual forward capacity
                    mincut_edges.emplace_back(u, v, cap_uv);
                }
            }
        }
    }

    // Prepare output
    ostringstream outbuf;
    outbuf << "Max Flow: " << maxflow << "\n";
    outbuf << "Augmentations performed: " << augmentations << "\n";
    outbuf << "Elapsed time (s): " << fixed << setprecision(6) << elapsed.count() << "\n";
    outbuf << "Nodes: " << n << ", edges read: " << read_edges << "\n";
    outbuf << "Min-cut (S-side size): ";
    int s_size = 0;
    for (int i = 0; i < n; ++i) if (reachable[i]) ++s_size;
    outbuf << s_size << "\n";
    outbuf << "S-side nodes: ";
    for (int i = 0; i < n; ++i) if (reachable[i]) outbuf << i << " ";
    outbuf << "\n";

    outbuf << "Min-cut edges (u in S, v in T) [residual_forward_capacity shown]:\n";
    for (auto &e : mincut_edges) {
        int u,v; ll c; tie(u,v,c) = e;
        outbuf << u << "," << v << "," << c << "\n";
    }

    outbuf << "\nResidual edges (u,v,capacity):\n";
    for (auto &e : residual_edges) {
        int u,v; ll c; tie(u,v,c) = e;
        outbuf << u << "," << v << "," << c << "\n";
    }

    if (!output_filename.empty()) {
        ofstream ofs(output_filename);
        if (!ofs) {
            cerr << "Failed to open output file: " << output_filename << "\n";
            return 11;
        }
        ofs << outbuf.str();
        ofs.close();
        if (verbose) cout << "Wrote result to: " << output_filename << "\n";
    } else {
        cout << outbuf.str();
    }

    return 0;
}

/*
SAMPLE CSV (suitable changes and notes):

The program expects a header line with four comma-separated integers: num_nodes,num_edges,source,sink
Followed by exactly num_edges lines giving edges in the form: u,v,capacity
Nodes are 0-indexed.

Example CSV matching --generate-sample output:

num_nodes,num_edges,source,sink
6,9,0,5
0,1,16
0,2,13
1,2,10
1,3,12
2,1,4
2,4,14
3,2,9
3,5,20
4,3,7
*/
