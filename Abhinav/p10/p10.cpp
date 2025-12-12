// lru_cache_ambulance.cpp
// LRU Cache implementation using Hash Map + Doubly Linked List.
// Simulates ambulance tablet caching most-recently accessed patient records.
//
// Compile: g++ -std=c++17 -O2 -pthread -o lru_cache_ambulance lru_cache_ambulance.cpp
//
// Usage: ./lru_cache_ambulance patient_accesses.csv [capacity=100] [--output cache_contents.csv]
//
// The program:
//  - Loads patient access CSV (patient_id, name, age, last_visit, access_timestamp, notes).
//  - Uses an LRU cache keyed by patient_id to keep most recent patient records.
//  - Simulates accesses by traversing the CSV and invoking cache.get(patient_id).
//  - Reports hit/miss statistics and writes final cache contents to CSV.

#include <bits/stdc++.h>
using namespace std;

// ------------------------ Patient record struct ---------------------------

struct PatientRecord {
    string patient_id;
    string name;
    int age;
    string last_visit;        // ISO timestamp string
    string access_timestamp;  // when accessed on the tablet
    string notes;

    // Simple CSV row for serialization
    string to_csv() const {
        auto quote = [](const string &s)->string {
            if (s.find(',') != string::npos || s.find('"') != string::npos) {
                string out = "\"";
                for (char c : s) {
                    if (c == '"') out += "\"\"";
                    else out += c;
                }
                out += "\"";
                return out;
            }
            return s;
        };
        return quote(patient_id) + "," + quote(name) + "," + to_string(age) + "," + quote(last_visit) + "," + quote(access_timestamp) + "," + quote(notes);
    }
};

// --------------------------- LRU Cache class ------------------------------

template<typename Key, typename Value>
class LRUCache {
public:
    // Doubly linked list node
    struct Node {
        Key key;
        Value value;
        Node *prev = nullptr;
        Node *next = nullptr;
        Node(const Key& k, const Value& v) : key(k), value(v) {}
    };

    LRUCache(size_t capacity) : capacity_(capacity), head_(nullptr), tail_(nullptr), size_(0) {
        if (capacity_ == 0) throw invalid_argument("Capacity must be > 0");
    }

    ~LRUCache() {
        clear();
    }

    // Thread-safe get (with mutex)
    bool get(const Key &key, Value &out) {
        lock_guard<mutex> lg(mtx_);
        auto it = map_.find(key);
        if (it == map_.end()) return false;
        Node* node = it->second;
        move_to_front(node);
        out = node->value;
        return true;
    }

    // Thread-safe put (insert or update)
    void put(const Key &key, const Value &value) {
        lock_guard<mutex> lg(mtx_);
        auto it = map_.find(key);
        if (it != map_.end()) {
            // update existing
            Node* node = it->second;
            node->value = value;
            move_to_front(node);
            return;
        }
        // insert new
        Node* node = new Node(key, value);
        add_to_front(node);
        map_[key] = node;
        ++size_;
        if (size_ > capacity_) {
            evict_lru();
        }
    }

    size_t size() const {
        lock_guard<mutex> lg(mtx_);
        return size_;
    }

    size_t capacity() const { return capacity_; }

    // Serialize current cache contents from most-recent->least-recent to CSV
    bool serialize_to_csv(const string &filename, const vector<string> &header = {}) {
        lock_guard<mutex> lg(mtx_);
        string tmp = filename + ".tmp";
        ofstream fout(tmp);
        if (!fout.is_open()) return false;
        if (!header.empty()) {
            for (size_t i = 0; i < header.size(); ++i) {
                fout << header[i];
                if (i+1 < header.size()) fout << ",";
            }
            fout << "\n";
        }
        Node* cur = head_;
        while (cur) {
            fout << cur->value.to_csv() << "\n";
            cur = cur->next;
        }
        fout.close();
        // atomic rename
        std::error_code ec;
        std::filesystem::rename(tmp, filename, ec);
        if (ec) {
            std::remove(filename.c_str());
            std::rename(tmp.c_str(), filename.c_str());
        }
        return true;
    }

    // For debugging: return vector of keys from most->least recent
    vector<Key> keys_most_to_least() const {
        lock_guard<mutex> lg(mtx_);
        vector<Key> out;
        Node* cur = head_;
        while (cur) { out.push_back(cur->key); cur = cur->next; }
        return out;
    }

    // Clear contents (free nodes)
    void clear() {
        lock_guard<mutex> lg(mtx_);
        Node* cur = head_;
        while (cur) {
            Node* nxt = cur->next;
            delete cur;
            cur = nxt;
        }
        head_ = tail_ = nullptr;
        map_.clear();
        size_ = 0;
    }

private:
    size_t capacity_;
    mutable mutex mtx_;
    unordered_map<Key, Node*> map_;
    Node* head_; // most recent
    Node* tail_; // least recent
    size_t size_;

    // Helper: add node to front (most recent)
    void add_to_front(Node* node) {
        node->prev = nullptr;
        node->next = head_;
        if (head_) head_->prev = node;
        head_ = node;
        if (!tail_) tail_ = node;
    }

    // Helper: move an existing node to front
    void move_to_front(Node* node) {
        if (node == head_) return;
        // detach
        if (node->prev) node->prev->next = node->next;
        if (node->next) node->next->prev = node->prev;
        if (node == tail_) tail_ = node->prev;
        // insert at front
        node->prev = nullptr;
        node->next = head_;
        if (head_) head_->prev = node;
        head_ = node;
        if (!tail_) tail_ = node;
    }

    // Evict least recently used node (tail)
    void evict_lru() {
        if (!tail_) return;
        Node* node = tail_;
        // remove from map
        map_.erase(node->key);
        // detach tail
        if (node->prev) {
            tail_ = node->prev;
            tail_->next = nullptr;
        } else {
            head_ = tail_ = nullptr;
        }
        delete node;
        --size_;
    }
};

// --------------------------- CSV loader -----------------------------------

bool parse_csv_line(const string &line, vector<string> &out) {
    out.clear();
    string cur;
    bool in_quotes = false;
    for (char ch : line) {
        if (ch == '"') { in_quotes = !in_quotes; continue; }
        if (ch == ',' && !in_quotes) {
            out.push_back(cur);
            cur.clear();
        } else cur.push_back(ch);
    }
    out.push_back(cur);
    for (auto &s : out) {
        // trim
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == string::npos) { s.clear(); continue; }
        size_t b = s.find_last_not_of(" \t\r\n");
        s = s.substr(a, b-a+1);
    }
    return true;
}

bool load_patient_accesses(const string &filename, vector<PatientRecord> &out, string &err) {
    ifstream fin(filename);
    if (!fin.is_open()) { err = "Cannot open file: " + filename; return false; }
    string header;
    if (!getline(fin, header)) { err = "Empty file"; return false; }
    vector<string> cols;
    parse_csv_line(header, cols);
    // find indices
    unordered_map<string,int> idx;
    for (int i=0;i<(int)cols.size();++i) {
        string key = cols[i];
        for (auto &c : key) c = tolower((unsigned char)c);
        // trim already done
        idx[key] = i;
    }
    auto find_col = [&](initializer_list<string> names)->int {
        for (auto &n : names) {
            string k = n;
            for (auto &c : k) c = tolower((unsigned char)c);
            if (idx.find(k) != idx.end()) return idx[k];
        }
        return -1;
    };
    int id_col = find_col({"patient_id","id"});
    int name_col = find_col({"name"});
    int age_col = find_col({"age"});
    int last_visit_col = find_col({"last_visit"});
    int access_col = find_col({"access_timestamp","access_time"});
    int notes_col = find_col({"notes"});
    string line;
    int lineno = 1;
    while (getline(fin, line)) {
        ++lineno;
        if (line.empty()) continue;
        vector<string> fields;
        parse_csv_line(line, fields);
        PatientRecord p;
        if (id_col >=0 && id_col < (int)fields.size()) p.patient_id = fields[id_col];
        if (name_col >=0 && name_col < (int)fields.size()) p.name = fields[name_col];
        if (age_col >=0 && age_col < (int)fields.size()) {
            try { p.age = stoi(fields[age_col]); } catch(...) { p.age = 0; }
        }
        if (last_visit_col >=0 && last_visit_col < (int)fields.size()) p.last_visit = fields[last_visit_col];
        if (access_col >=0 && access_col < (int)fields.size()) p.access_timestamp = fields[access_col];
        if (notes_col >=0 && notes_col < (int)fields.size()) p.notes = fields[notes_col];
        out.push_back(p);
    }
    fin.close();
    return true;
}

// ---------------------------- Main demo -----------------------------------

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " patient_accesses.csv [capacity=100] [--output cache_contents.csv]\n";
        return 1;
    }
    string infile = argv[1];
    size_t capacity = 100;
    string outfile = "cache_contents.csv";
    for (int i=2;i<argc;++i) {
        string s = argv[i];
        if (s == "--output" && i+1<argc) outfile = argv[++i];
        else {
            try { capacity = stoul(s); } catch(...) {}
        }
    }

    vector<PatientRecord> accesses;
    string err;
    if (!load_patient_accesses(infile, accesses, err)) {
        cerr << "Error loading patient accesses: " << err << "\n";
        return 1;
    }
    cout << "Loaded " << accesses.size() << " patient access records.\n";
    LRUCache<string, PatientRecord> cache(capacity);
    size_t hits = 0, misses = 0;

    // Simulate accesses: treat each entry as an access; try get, otherwise load (put)
    for (const auto &p : accesses) {
        PatientRecord rec;
        bool found = cache.get(p.patient_id, rec);
        if (found) {
            ++hits;
            // simulate update of access_timestamp
            rec.access_timestamp = p.access_timestamp;
            cache.put(p.patient_id, rec); // move to front and update timestamp
        } else {
            ++misses;
            // load record into cache (simulate fetching from server or DB)
            cache.put(p.patient_id, p);
        }
    }

    cout << "Simulation complete. Cache capacity=" << capacity << ", final size=" << cache.size() << "\n";
    cout << "Hits=" << hits << ", Misses=" << misses << ", Hit ratio=" << fixed << setprecision(3) << (double)hits / (hits + misses) << "\n";

    // Serialize cache contents
    vector<string> header = {"patient_id","name","age","last_visit","access_timestamp","notes"};
    if (cache.serialize_to_csv(outfile, header)) {
        cout << "Wrote cache contents to " << outfile << "\n";
    } else {
        cerr << "Failed to write cache contents to " << outfile << "\n";
    }

    // For demonstration, print top 10 most-recent patient IDs
    auto keys = cache.keys_most_to_least();
    cout << "Most-recently-used patient IDs (top 10):\n";
    for (size_t i = 0; i < keys.size() && i < 10; ++i) cout << "  " << keys[i] << "\n";

    return 0;
}
