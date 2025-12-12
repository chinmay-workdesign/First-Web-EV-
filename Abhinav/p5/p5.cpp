// autocomplete_trie.cpp
// Autocomplete system using Trie (prefix tree).
// Compile: g++ -std=c++17 -O2 -pthread -o autocomplete_trie autocomplete_trie.cpp
//
// Input: CSV with columns id,name,address  (generated sample: names_addresses.csv)
// Features:
//  - Insert entries (name + address as a single string) into trie
//  - Query top-k suggestions by prefix (case-insensitive)
//  - Maintain frequency counts for ranking suggestions
//  - Support deletion of entries
//  - Optional fuzzy fallback using simple Levenshtein scan for small datasets
//  - Interactive REPL and batch query mode
//
// Note: This is an educational implementation; production systems use optimised
// tries (radix/compressed), disk-backed stores, and persistence.

#include <bits/stdc++.h>
using namespace std;

// --------------------------- Utilities ------------------------------------

static inline void trim(string &s) {
    const char* ws = " \t\r\n";
    size_t a = s.find_first_not_of(ws);
    if (a == string::npos) { s.clear(); return; }
    size_t b = s.find_last_not_of(ws);
    s = s.substr(a, b - a + 1);
}

static inline string to_lower_normalize(const string &s) {
    string out;
    out.reserve(s.size());
    for (char ch : s) {
        // normalize: convert to lowercase, keep alphanum and basic punctuation/spaces
        char c = ch;
        if (unsigned char(c) >= 128) {
            // skip non-ascii accents for simplicity; production should use unicode normalization
            continue;
        }
        if (isalpha((unsigned char)c)) out.push_back(tolower((unsigned char)c));
        else if (isdigit((unsigned char)c)) out.push_back(c);
        else if (isspace((unsigned char)c)) out.push_back(' ');
        else if (c == ',' || c == '.' || c == '-' || c == '&' || c == '/') out.push_back(c);
        // else skip other symbols
    }
    // collapse multiple spaces
    string res;
    bool was_space = false;
    for (char ch : out) {
        if (ch == ' ') {
            if (!was_space) { res.push_back(ch); was_space = true; }
        } else {
            res.push_back(ch);
            was_space = false;
        }
    }
    trim(res);
    return res;
}

// --------------------------- Trie Node ------------------------------------

struct Suggestion {
    string key;      // full string (e.g., "Name, Address")
    uint64_t freq;   // frequency or score (higher -> more relevant)
    uint64_t last_ts; // last-used timestamp for recency tie-break
};

struct TrieNode {
    // For simplicity using unordered_map for children to support large char set.
    unordered_map<char, TrieNode*> children;
    bool is_end = false;
    // if node corresponds to an entry end, store an index to suggestion store
    vector<int> suggestion_indices; // indices into global suggestion pool for entries ending here (usually 1)
    // We also keep top suggestions cache for this subtree to accelerate queries
    // (store indices to global suggestions and maintain it upon inserts/deletes)
    vector<int> top_cache; // stores indices into global suggestions, sorted by score
    mutable std::shared_mutex node_mutex; // to allow concurrent reads and safe updates
};

// --------------------------- Global store ---------------------------------

// Global vector of suggestions (strings and metadata)
vector<Suggestion> suggestion_store;

// Map from key string to index in suggestion_store
unordered_map<string, int> key_to_index;

// Root of trie
TrieNode *root = nullptr;

// Parameters
size_t MAX_CACHE_PER_NODE = 10; // how many top suggestions we cache per node

// Lock for suggestion store writes
std::mutex store_mutex;

// --------------------------- Ranking helpers -------------------------------

// Compute score for suggestion: primary freq, secondary recent ts
uint64_t compute_score(const Suggestion &s) {
    // Combine frequency and timestamp in 64-bit score (freq high bits)
    // freq up to 2^48 and last_ts up to 2^16 approx; here simple combination:
    // score = (freq << 32) | (last_ts & 0xffffffff)
    return (s.freq << 32) | (s.last_ts & 0xffffffff);
}

// Compare two indices by suggestion score (higher first), fallback lexicographic
bool suggestion_cmp_idx(int a, int b) {
    const Suggestion &sa = suggestion_store[a];
    const Suggestion &sb = suggestion_store[b];
    uint64_t ra = compute_score(sa);
    uint64_t rb = compute_score(sb);
    if (ra != rb) return ra > rb;
    if (sa.key != sb.key) return sa.key < sb.key;
    return a < b;
}

// Merge top caches: node cache is kept up to MAX_CACHE_PER_NODE best suggestions
void merge_into_cache(vector<int> &cache, int idx) {
    // if idx already present, update it
    for (int &x : cache) {
        if (x == idx) {
            // resort
            sort(cache.begin(), cache.end(), suggestion_cmp_idx);
            if (cache.size() > MAX_CACHE_PER_NODE) cache.resize(MAX_CACHE_PER_NODE);
            return;
        }
    }
    cache.push_back(idx);
    sort(cache.begin(), cache.end(), suggestion_cmp_idx);
    if (cache.size() > MAX_CACHE_PER_NODE) cache.resize(MAX_CACHE_PER_NODE);
}

// --------------------------- Trie operations -------------------------------

TrieNode* make_node() {
    TrieNode* node = new TrieNode();
    return node;
}

// Insert key into trie and update suggestion store; returns index in store
int insert_suggestion(const string &raw_key, uint64_t timestamp = 0) {
    string key = to_lower_normalize(raw_key);
    lock_guard<std::mutex> lg(store_mutex);
    int idx;
    auto it = key_to_index.find(key);
    if (it != key_to_index.end()) {
        // exists -> increment freq and update ts
        idx = it->second;
        suggestion_store[idx].freq += 1;
        suggestion_store[idx].last_ts = timestamp;
    } else {
        idx = (int)suggestion_store.size();
        suggestion_store.push_back({raw_key, 1, timestamp});
        key_to_index.emplace(key, idx);
    }
    // insert into trie and update caches along the path
    TrieNode* node = root;
    // lock-free traversal not safe for inserts; we will use node mutexes when updating
    string norm = key;
    for (char ch : norm) {
        std::unique_lock<std::shared_mutex> lock(node->node_mutex);
        TrieNode* next;
        auto itc = node->children.find(ch);
        if (itc == node->children.end()) {
            next = make_node();
            node->children[ch] = next;
        } else next = itc->second;
        // update cache at node
        merge_into_cache(node->top_cache, idx);
        lock.unlock();
        node = next;
    }
    {
        std::unique_lock<std::shared_mutex> lock(node->node_mutex);
        node->is_end = true;
        node->suggestion_indices.push_back(idx);
        merge_into_cache(node->top_cache, idx);
    }
    return idx;
}

// Delete a suggestion (decrement frequency, optionally remove if freq==0)
bool delete_suggestion(const string &raw_key) {
    string key = to_lower_normalize(raw_key);
    lock_guard<std::mutex> lg(store_mutex);
    auto it = key_to_index.find(key);
    if (it == key_to_index.end()) return false;
    int idx = it->second;
    // decrement frequency
    if (suggestion_store[idx].freq > 1) {
        suggestion_store[idx].freq -= 1;
        suggestion_store[idx].last_ts = 0;
        // Ideally update caches along path (we skip for simplicity)
        return true;
    }
    // freq==1 -> remove suggestion entirely
    suggestion_store[idx].freq = 0;
    // mark removed; don't reindex suggestion_store to avoid reindexing costs
    suggestion_store[idx].key = ""; // mark deleted
    key_to_index.erase(it);
    // Note: trie cleanup (removing nodes) is complex and optional;
    // We'll leave nodes present but entries removed from suggestion_indices and caches lazily.
    return true;
}

// Find node corresponding to prefix (normalized), return nullptr if not found
TrieNode* find_node_for_prefix(const string &prefix) {
    TrieNode* node = root;
    string norm = to_lower_normalize(prefix);
    for (char ch : norm) {
        std::shared_lock<std::shared_mutex> lock(node->node_mutex);
        auto it = node->children.find(ch);
        if (it == node->children.end()) return nullptr;
        node = it->second;
    }
    return node;
}

// Gather top suggestions under node using cache, fallback to DFS if necessary
vector<int> gather_top_suggestions(TrieNode* node, int top_k) {
    vector<int> result;
    if (!node) return result;
    { // try using cache
        std::shared_lock<std::shared_mutex> lock(node->node_mutex);
        for (int idx : node->top_cache) {
            // filter out deleted entries
            if (idx >= 0 && idx < (int)suggestion_store.size() && !suggestion_store[idx].key.empty()) {
                result.push_back(idx);
                if ((int)result.size() >= top_k) break;
            }
        }
        if ((int)result.size() >= top_k) return result;
    }
    // Cache insufficient -> do DFS to find more suggestions
    // BFS traversal collecting suggestion indices and using priority by score
    priority_queue<int, vector<int>, function<bool(int,int)>> pq(
        [](int a, int b){ return suggestion_cmp_idx(a,b); } // not used but prepare
    );
    // we'll use a vector and then sort
    vector<int> found;
    // BFS queue
    deque<TrieNode*> q;
    q.push_back(node);
    while (!q.empty() && (int)found.size() < top_k*5) { // limit exploration to avoid heavy work
        TrieNode* cur = q.front(); q.pop_front();
        std::shared_lock<std::shared_mutex> lock(cur->node_mutex);
        // add end suggestions
        for (int idx : cur->suggestion_indices) {
            if (idx >= 0 && idx < (int)suggestion_store.size() && !suggestion_store[idx].key.empty())
                found.push_back(idx);
        }
        for (auto &p : cur->children) q.push_back(p.second);
    }
    // sort found by score and pick top_k
    sort(found.begin(), found.end(), suggestion_cmp_idx);
    for (int i = 0; i < (int)found.size() && (int)result.size() < top_k; ++i) {
        result.push_back(found[i]);
    }
    return result;
}

// Autocomplete API: returns vector of suggestion strings (top_k)
vector<Suggestion> autocomplete(const string &prefix, int top_k) {
    vector<Suggestion> out;
    TrieNode* node = find_node_for_prefix(prefix);
    if (!node) return out;
    vector<int> idxs = gather_top_suggestions(node, top_k);
    for (int idx : idxs) {
        out.push_back(suggestion_store[idx]);
    }
    return out;
}

// --------------------------- CSV Loading ----------------------------------

bool load_csv_and_build_trie(const string &filename) {
    ifstream fin(filename);
    if (!fin.is_open()) {
        cerr << "Cannot open file: " << filename << "\n";
        return false;
    }
    string header;
    if (!getline(fin, header)) {
        cerr << "Empty file\n"; return false;
    }
    // simple parse: expect columns id,name,address
    string line;
    int line_no = 1;
    while (getline(fin, line)) {
        ++line_no;
        if (line.empty()) continue;
        // naive split into 3 columns - handle commas in address by splitting first two commas
        vector<string> parts;
        parts.reserve(3);
        string cur;
        int commas = 0;
        for (char ch : line) {
            if (ch == ',' && commas < 2) { parts.push_back(cur); cur.clear(); ++commas; }
            else cur.push_back(ch);
        }
        parts.push_back(cur);
        for (auto &p : parts) { trim(p); }
        if (parts.size() < 3) continue;
        string id = parts[0];
        string name = parts[1];
        string address = parts[2];
        string key = name + ", " + address;
        // use current timestamp as index (monotonic)
        uint64_t ts = (uint64_t)time(nullptr);
        insert_suggestion(key, ts);
    }
    fin.close();
    return true;
}

// --------------------------- Levenshtein (simple fallback) -----------------

int levenshtein(const string &a, const string &b) {
    int n = a.size(), m = b.size();
    vector<int> dp(m+1);
    for (int j=0;j<=m;++j) dp[j]=j;
    for (int i=1;i<=n;++i) {
        int prev = dp[0];
        dp[0] = i;
        for (int j=1;j<=m;++j) {
            int cur = dp[j];
            int cost = (a[i-1]==b[j-1]) ? 0 : 1;
            dp[j] = min({ dp[j]+1, dp[j-1]+1, prev + cost });
            prev = cur;
        }
    }
    return dp[m];
}

// Fuzzy fallback: scan all suggestions and return those with small edit distance (only for small datasets)
vector<Suggestion> fuzzy_suggest(const string &prefix, int top_k) {
    string norm = to_lower_normalize(prefix);
    vector<pair<int,int>> candidates; // distance, idx
    for (int i=0;i<(int)suggestion_store.size();++i) {
        if (suggestion_store[i].key.empty()) continue;
        string k = to_lower_normalize(suggestion_store[i].key);
        int d = levenshtein(norm, k.substr(0, min((int)k.size(), (int)norm.size()+2)));
        candidates.emplace_back(d, i);
    }
    sort(candidates.begin(), candidates.end());
    vector<Suggestion> out;
    for (int i=0;i<(int)candidates.size() && (int)out.size()<top_k; ++i) {
        out.push_back(suggestion_store[candidates[i].second]);
    }
    return out;
}

// --------------------------- Main (CLI) -----------------------------------

void print_usage() {
    cerr << "Usage: autocomplete_trie data.csv [--top K] [--fuzzy]\n";
    cerr << "Then type prefixes interactively to get suggestions (type exit to quit).\n";
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc < 2) { print_usage(); return 1; }
    string datafile = argv[1];
    int top_k = 5;
    bool fuzzy = false;
    for (int i=2;i<argc;++i) {
        string s = argv[i];
        if (s == "--top" && i+1<argc) top_k = stoi(argv[++i]);
        else if (s == "--fuzzy") fuzzy = true;
    }

    // Initialize root
    root = make_node();

    cout << "Loading data from " << datafile << " ...\n";
    if (!load_csv_and_build_trie(datafile)) {
        cerr << "Failed to load CSV\n";
        return 1;
    }
    cout << "Loaded " << suggestion_store.size() << " suggestions into trie.\n";
    cout << "Ready. Enter prefix queries (type 'exit' or blank line to quit).\n";

    string line;
    while (true) {
        cout << "> ";
        if (!getline(cin, line)) break;
        trim(line);
        if (line.empty()) break;
        if (line == "exit" || line == "quit") break;
        // if user types a number to select suggestion, not implemented here
        auto suggestions = autocomplete(line, top_k);
        if (suggestions.empty() && fuzzy) {
            auto f = fuzzy_suggest(line, top_k);
            if (!f.empty()) {
                cout << "Fuzzy suggestions:\n";
                for (auto &s : f) cout << "  " << s.key << "  (freq=" << s.freq << ")\n";
                continue;
            }
        }
        if (suggestions.empty()) {
            cout << "No suggestions.\n";
            continue;
        }
        cout << "Top " << suggestions.size() << " suggestions:\n";
        for (auto &s : suggestions) {
            cout << "  " << s.key << "  (freq=" << s.freq << ", last=" << s.last_ts << ")\n";
        }
    }

    cout << "Exiting.\n";
    return 0;
}
