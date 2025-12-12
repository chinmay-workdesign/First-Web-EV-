// ring_buffer_calls.cpp
// Circular Ring Buffer to keep exactly the last K call records for auditing.
//
// Compile: g++ -std=c++17 -O2 -pthread -o ring_buffer_calls ring_buffer_calls.cpp
//
// Usage: ./ring_buffer_calls calls.csv [K=100] [--output audit_last_k.csv]
//
// The program reads a CSV of call logs (call_id,caller,callee,timestamp,duration_seconds,status),
// streams them into a fixed-size ring buffer of capacity K, and writes the final buffer
// contents (oldest->newest) to an output CSV.

#include <bits/stdc++.h>
#include <atomic>
#include <shared_mutex>
using namespace std;

// --------------------------- Call record ----------------------------------

struct CallRecord {
    string call_id;
    string caller;
    string callee;
    string timestamp; // keep as string for audit; parsing optionally available
    int duration_seconds = 0;
    string status;

    string to_csv_row() const {
        // naive CSV quoting (fields containing commas will be quoted)
        auto quote = [](const string &s)->string{
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
        return quote(call_id) + "," + quote(caller) + "," + quote(callee) + "," + quote(timestamp) + "," + to_string(duration_seconds) + "," + quote(status);
    }
};

// --------------------------- Ring buffer ----------------------------------

template<typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity)
        : capacity_(capacity), buffer_(capacity), next_index_(0), full_(false)
    {
        if (capacity_ == 0) throw invalid_argument("capacity must be > 0");
    }

    // Push an item into buffer; overwrites oldest when full.
    // Thread-safe for concurrent producers via mutex (coarse-grained).
    void push(const T &item) {
        unique_lock<shared_mutex> lock(mutex_);
        buffer_[next_index_] = item;
        next_index_ = (next_index_ + 1) % capacity_;
        if (next_index_ == 0) full_ = true;
    }

    // Return number of items currently in buffer
    size_t size() const {
        shared_lock<shared_mutex> lock(mutex_);
        return full_ ? capacity_ : next_index_;
    }

    // Return buffer capacity
    size_t capacity() const { return capacity_; }

    // Clear buffer
    void clear() {
        unique_lock<shared_mutex> lock(mutex_);
        for (size_t i = 0; i < capacity_; ++i) buffer_[i] = T();
        next_index_ = 0;
        full_ = false;
    }

    // Retrieve items ordered from oldest to newest
    vector<T> get_all() const {
        shared_lock<shared_mutex> lock(mutex_);
        vector<T> out;
        size_t sz = size();
        out.reserve(sz);
        if (sz == 0) return out;
        size_t start = full_ ? next_index_ : 0;
        for (size_t i = 0; i < sz; ++i) {
            size_t idx = (start + i) % capacity_;
            out.push_back(buffer_[idx]);
        }
        return out;
    }

    // Random access: index 0 = oldest, index size()-1 = newest. Throws if out of range.
    T get_at_oldest_index(size_t idx) const {
        shared_lock<shared_mutex> lock(mutex_);
        size_t sz = size();
        if (idx >= sz) throw out_of_range("index out of range");
        size_t start = full_ ? next_index_ : 0;
        size_t real_idx = (start + idx) % capacity_;
        return buffer_[real_idx];
    }

    // Write buffer contents to CSV file (atomic write via temp file then rename)
    bool serialize_to_csv(const string &filename, const vector<string> &header = {}) const {
        string tmp = filename + ".tmp";
        ofstream fout(tmp, ios::binary);
        if (!fout.is_open()) return false;
        // header
        if (!header.empty()) {
            for (size_t i = 0; i < header.size(); ++i) {
                fout << header[i];
                if (i+1 < header.size()) fout << ",";
            }
            fout << "\n";
        }
        auto all = get_all();
        for (auto &rec : all) fout << rec.to_csv_row() << "\n";
        fout.close();
        // atomic rename
        std::error_code ec;
        std::filesystem::rename(tmp, filename, ec);
        if (ec) {
            // fallback: try remove and rename
            std::remove(filename.c_str());
            std::rename(tmp.c_str(), filename.c_str());
        }
        return true;
    }

private:
    size_t capacity_;
    mutable shared_mutex mutex_;
    vector<T> buffer_;
    size_t next_index_; // index where next write will happen (0..capacity-1)
    bool full_;
};

// --------------------------- CSV loader -----------------------------------

bool parse_csv_row_simple(const string &line, vector<string> &cols) {
    cols.clear();
    string cur;
    bool in_quotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"' ) {
            in_quotes = !in_quotes;
            continue;
        }
        if (c == ',' && !in_quotes) {
            cols.push_back(cur);
            cur.clear();
        } else cur.push_back(c);
    }
    cols.push_back(cur);
    for (auto &s : cols) {
        // trim spaces
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == string::npos) { s.clear(); continue; }
        size_t b = s.find_last_not_of(" \t\r\n");
        s = s.substr(a, b-a+1);
    }
    return true;
}

// Load CSV of call logs into vector of CallRecord
bool load_calls_csv(const string &filename, vector<CallRecord> &out, string &err) {
    ifstream fin(filename);
    if (!fin.is_open()) { err = "Cannot open file: " + filename; return false; }
    string header;
    if (!getline(fin, header)) { err = "Empty file"; return false; }
    // header columns expected: call_id,caller,callee,timestamp,duration_seconds,status
    vector<string> cols;
    parse_csv_row_simple(header, cols);
    unordered_map<string,int> col_index;
    for (int i = 0; i < (int)cols.size(); ++i) {
        string key = cols[i];
        for (char &ch : key) ch = (char)tolower((unsigned char)ch);
        // trim was done
        col_index[key] = i;
    }
    auto find_col = [&](const vector<string> &cands)->int{
        for (auto &c : cands) {
            string k = c;
            for (char &ch : k) ch = (char)tolower((unsigned char)ch);
            if (col_index.find(k) != col_index.end()) return col_index[k];
        }
        return -1;
    };
    int id_col = find_col({"call_id","id"});
    int caller_col = find_col({"caller","from"});
    int callee_col = find_col({"callee","to"});
    int ts_col = find_col({"timestamp","time","datetime"});
    int dur_col = find_col({"duration_seconds","duration","length"});
    int status_col = find_col({"status","state"});
    string line;
    int lineno = 1;
    while (getline(fin, line)) {
        ++lineno;
        if (line.empty()) continue;
        vector<string> fields;
        parse_csv_row_simple(line, fields);
        CallRecord rec;
        if (id_col >= 0 && id_col < (int)fields.size()) rec.call_id = fields[id_col];
        if (caller_col >=0 && caller_col < (int)fields.size()) rec.caller = fields[caller_col];
        if (callee_col >=0 && callee_col < (int)fields.size()) rec.callee = fields[callee_col];
        if (ts_col >=0 && ts_col < (int)fields.size()) rec.timestamp = fields[ts_col];
        if (dur_col >=0 && dur_col < (int)fields.size()) {
            try { rec.duration_seconds = stoi(fields[dur_col]); } catch(...) { rec.duration_seconds = 0; }
        }
        if (status_col >=0 && status_col < (int)fields.size()) rec.status = fields[status_col];
        out.push_back(rec);
    }
    fin.close();
    return true;
}

// ----------------------------- Demo main ----------------------------------

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " calls.csv [K=100] [--output audit_last_k.csv]\n";
        return 1;
    }
    string infile = argv[1];
    size_t K = 100;
    string outfile = "audit_last_k.csv";
    for (int i = 2; i < argc; ++i) {
        string s = argv[i];
        if (s == "--output" && i+1 < argc) { outfile = argv[++i]; }
        else {
            // try parse K
            try { K = stoul(s); } catch(...) { /* ignore */ }
        }
    }

    vector<CallRecord> calls;
    string err;
    if (!load_calls_csv(infile, calls, err)) {
        cerr << "Error loading calls: " << err << "\n";
        return 1;
    }
    cout << "Loaded " << calls.size() << " call records from " << infile << "\n";
    cout << "Initializing ring buffer with capacity K=" << K << "\n";

    RingBuffer<CallRecord> ring(K);

    // Simulate streaming ingestion: push all calls into ring buffer.
    for (size_t i = 0; i < calls.size(); ++i) {
        ring.push(calls[i]);
    }

    cout << "After ingestion, buffer size = " << ring.size() << " (<= K)\n";

    // Retrieve oldest->newest and print first 10
    auto all = ring.get_all();
    cout << "Oldest -> Newest (showing up to 10):\n";
    for (size_t i = 0; i < all.size() && i < 10; ++i) {
        const auto &r = all[i];
        cout << i << ": " << r.call_id << " | " << r.caller << " -> " << r.callee << " | " << r.timestamp << " | dur=" << r.duration_seconds << " | " << r.status << "\n";
    }

    // Save audit file
    vector<string> header = {"call_id","caller","callee","timestamp","duration_seconds","status"};
    if (ring.serialize_to_csv(outfile, header)) {
        cout << "Wrote last " << ring.size() << " calls to " << outfile << "\n";
    } else {
        cerr << "Failed to write output file " << outfile << "\n";
    }

    // Example: random access: print newest entry
    if (ring.size() > 0) {
        size_t sz = ring.size();
        try {
            auto newest = ring.get_at_oldest_index(sz - 1);
            cout << "Newest entry: " << newest.call_id << " at " << newest.timestamp << "\n";
        } catch (const exception &e) {
            cerr << "Random access error: " << e.what() << "\n";
        }
    }

    return 0;
}
