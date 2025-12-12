// dedupe_incidents.cpp
// Deduplicate 911 calls for the same incident using HashSet + HashMap + TTL heap.
//
// Compile: g++ -std=c++17 -O2 -o dedupe_incidents dedupe_incidents.cpp
//
// Usage: ./dedupe_incidents calls_with_duplicates.csv [--ttl SECONDS] [--output active_incidents.csv]
//
// The program demonstrates:
//  - computing stable dedupe keys (grid + type)
//  - detecting duplicate calls in O(1) average time
//  - aggregating metadata per incident
//  - expiring old incidents via a min-heap (priority queue)
//  - exporting active incident summary

#include <bits/stdc++.h>
using namespace std;
using ll = long long;

// --------------------------- Data structures -------------------------------

struct Call {
    string call_id;
    string incident_id; // reporter-supplied incident id, may be absent or noisy
    double latitude;
    double longitude;
    string reported_type;
    string timestamp_str;
    string caller;
    double confidence;
    string raw_hash_key; // included in CSV for debugging
};

struct Incident {
    string key; // dedupe key (grid_type)
    string created_at; // timestamp string of first call
    time_t first_seen_epoch = 0;
    time_t last_seen_epoch = 0;
    string reported_type;
    double repr_lat = 0.0; // representative latitude (first call)
    double repr_lon = 0.0;
    vector<string> call_ids; // aggregated call ids
    int call_count = 0;
    bool active = true;
};

// Heap entry for expiry: (expiry_time, sequence, key)
// sequence avoids ambiguity for equal expiry times.
struct ExpiryEntry {
    time_t expiry;
    uint64_t seq;
    string key;
    bool operator>(const ExpiryEntry &o) const {
        if (expiry != o.expiry) return expiry > o.expiry;
        return seq > o.seq;
    }
};

// --------------------------- Helpers --------------------------------------

static inline void trim(string &s) {
    const char* ws = " \t\n\r";
    size_t a = s.find_first_not_of(ws);
    if (a == string::npos) { s.clear(); return; }
    size_t b = s.find_last_not_of(ws);
    s = s.substr(a, b - a + 1);
}

// parse a basic ISO-like timestamp to epoch seconds (naive, localtime)
// supports "YYYY-MM-DD HH:MM:SS" or "YYYY-MM-DD HH:MM"
bool parse_iso_to_epoch(const string &s, time_t &out_epoch) {
    string t = s;
    for (char &c : t) if (c == 'T') c = ' ';
    trim(t);
    if (t.empty()) return false;
    // split date and time
    auto pos = t.find(' ');
    if (pos == string::npos) return false;
    string date = t.substr(0, pos);
    string time = t.substr(pos+1);
    vector<int> dparts;
    vector<int> tparts;
    {
        string cur;
        for (char c : date) {
            if (c == '-') { dparts.push_back(stoi(cur)); cur.clear(); }
            else cur.push_back(c);
        }
        if (!cur.empty()) dparts.push_back(stoi(cur));
    }
    {
        string cur;
        for (char c : time) {
            if (c == ':') { tparts.push_back(stoi(cur)); cur.clear(); }
            else if (isdigit((unsigned char)c)) cur.push_back(c);
            else break;
        }
        if (!cur.empty()) tparts.push_back(stoi(cur));
    }
    if (dparts.size() != 3 || tparts.size() < 2) return false;
    int year=dparts[0], month=dparts[1], day=dparts[2];
    int hour=tparts[0], minute=tparts[1], second=(tparts.size()>=3?tparts[2]:0);
    struct tm tm_time;
    memset(&tm_time,0,sizeof(tm_time));
    tm_time.tm_year = year - 1900;
    tm_time.tm_mon = month - 1;
    tm_time.tm_mday = day;
    tm_time.tm_hour = hour;
    tm_time.tm_min = minute;
    tm_time.tm_sec = second;
    tm_time.tm_isdst = -1;
    time_t epoch = mktime(&tm_time);
    if (epoch == (time_t)-1) return false;
    out_epoch = epoch;
    return true;
}

// Quantize coordinates to coarse grid (3 decimal places ~ ~110m latitude, variable longitude)
static inline string grid_key(double lat, double lon, int decimals = 3) {
    double glat = round(lat * pow(10, decimals)) / pow(10, decimals);
    double glon = round(lon * pow(10, decimals)) / pow(10, decimals);
    char buf[64];
    snprintf(buf, sizeof(buf), "%.3f_%.3f", glat, glon);
    return string(buf);
}

// Build dedupe key from call: grid + type
static inline string dedupe_key_from_call(const Call &c) {
    string g = grid_key(c.latitude, c.longitude, 3);
    return g + "|" + c.reported_type;
}

// ------------------------ CSV loader --------------------------------------

bool parse_csv_row(const string &line, vector<string> &out) {
    out.clear();
    string cur;
    bool in_quote = false;
    for (size_t i=0;i<line.size();++i) {
        char ch = line[i];
        if (ch == '"') { in_quote = !in_quote; continue; }
        if (ch == ',' && !in_quote) { out.push_back(cur); cur.clear(); }
        else cur.push_back(ch);
    }
    out.push_back(cur);
    for (auto &s : out) { trim(s); }
    return true;
}

// Expect CSV columns exactly as generated: call_id,incident_id,latitude,longitude,reported_type,timestamp,caller,confidence,hash_key
bool load_calls_csv(const string &filename, vector<Call> &out, string &err) {
    ifstream fin(filename);
    if (!fin.is_open()) { err = "Cannot open " + filename; return false; }
    string header;
    if (!getline(fin, header)) { err = "Empty file"; return false; }
    vector<string> cols;
    parse_csv_row(header, cols);
    // map known columns to indices
    unordered_map<string,int> idx;
    for (int i=0;i<(int)cols.size();++i) {
        string key = cols[i];
        for (char &c : key) c = tolower((unsigned char)c);
        trim(key);
        idx[key] = i;
    }
    auto find_col = [&](initializer_list<string> names)->int {
        for (auto &n : names) {
            string k = n;
            for (char &c : k) c = tolower((unsigned char)c);
            if (idx.find(k) != idx.end()) return idx[k];
        }
        return -1;
    };
    int call_id_col = find_col({"call_id","id"});
    int incident_col = find_col({"incident_id","report_id","report"});
    int lat_col = find_col({"latitude","lat"});
    int lon_col = find_col({"longitude","lon"});
    int type_col = find_col({"reported_type","type"});
    int ts_col = find_col({"timestamp","time"});
    int caller_col = find_col({"caller","from"});
    int conf_col = find_col({"confidence"});
    int hash_col = find_col({"hash_key"});
    string line;
    int lineno = 1;
    while (getline(fin, line)) {
        ++lineno;
        if (line.empty()) continue;
        vector<string> fields;
        if (!parse_csv_row(line, fields)) continue;
        Call c;
        if (call_id_col >=0 && call_id_col < (int)fields.size()) c.call_id = fields[call_id_col];
        if (incident_col >=0 && incident_col < (int)fields.size()) c.incident_id = fields[incident_col];
        if (lat_col >=0 && lat_col < (int)fields.size()) {
            try { c.latitude = stod(fields[lat_col]); } catch(...) { c.latitude = 0.0; }
        }
        if (lon_col >=0 && lon_col < (int)fields.size()) {
            try { c.longitude = stod(fields[lon_col]); } catch(...) { c.longitude = 0.0; }
        }
        if (type_col >=0 && type_col < (int)fields.size()) c.reported_type = fields[type_col];
        if (ts_col >=0 && ts_col < (int)fields.size()) c.timestamp_str = fields[ts_col];
        if (caller_col >=0 && caller_col < (int)fields.size()) c.caller = fields[caller_col];
        if (conf_col >=0 && conf_col < (int)fields.size()) {
            try { c.confidence = stod(fields[conf_col]); } catch(...) { c.confidence = 1.0; }
        }
        if (hash_col >=0 && hash_col < (int)fields.size()) c.raw_hash_key = fields[hash_col];
        out.push_back(c);
    }
    fin.close();
    return true;
}

// ------------------------ Deduplication engine -----------------------------

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " calls_with_duplicates.csv [--ttl seconds] [--output active_incidents.csv]\n";
        return 1;
    }
    string infile = argv[1];
    int TTL = 300; // seconds default time-to-live for an incident to be considered active
    string outfile = "active_incidents.csv";
    for (int i=2;i<argc;++i) {
        string s = argv[i];
        if (s == "--ttl" && i+1 < argc) { TTL = stoi(argv[++i]); }
        else if (s == "--output" && i+1 < argc) { outfile = argv[++i]; }
    }

    vector<Call> calls;
    string err;
    if (!load_calls_csv(infile, calls, err)) {
        cerr << "Error: " << err << "\n";
        return 1;
    }
    cout << "Loaded " << calls.size() << " calls from " << infile << "\n";
    // Active structures
    unordered_set<string> active_keys;                  // set of dedupe keys for quick test
    unordered_map<string, Incident> incidents_map;     // key -> incident metadata
    // min-heap for expiry
    priority_queue<ExpiryEntry, vector<ExpiryEntry>, greater<ExpiryEntry>> expiry_heap;
    uint64_t seq_counter = 0;

    size_t duplicate_count = 0;
    size_t new_incident_count = 0;

    for (const auto &c : calls) {
        time_t epoch;
        bool parsed_time = parse_iso_to_epoch(c.timestamp_str, epoch);
        if (!parsed_time) epoch = time(nullptr); // fallback
        // purge expired incidents before processing this call
        while (!expiry_heap.empty()) {
            auto top = expiry_heap.top();
            if (top.expiry <= epoch) {
                expiry_heap.pop();
                // check if incident still present and last_seen <= expiry - TTL (stale)
                auto it = incidents_map.find(top.key);
                if (it != incidents_map.end()) {
                    // If last_seen_epoch <= top.expiry - TTL => no later renewal, remove
                    if (it->second.last_seen_epoch + TTL <= top.expiry) {
                        // expire
                        active_keys.erase(top.key);
                        it->second.active = false;
                        // optionally write to persistent store or archive here
                    } else {
                        // renewed after this expiry was scheduled; skip
                    }
                }
                continue;
            }
            break;
        }

        string key = dedupe_key_from_call(c);
        auto it = incidents_map.find(key);
        if (it != incidents_map.end() && it->second.active) {
            // duplicate call — update metadata
            duplicate_count++;
            Incident &inc = it->second;
            inc.call_ids.push_back(c.call_id);
            inc.call_count += 1;
            inc.last_seen_epoch = epoch;
            inc.created_at = inc.created_at; // unchanged
            // extend expiry by pushing a new expiry event
            seq_counter++;
            expiry_heap.push({epoch + TTL, seq_counter, key});
        } else {
            // new incident
            new_incident_count++;
            Incident inc;
            inc.key = key;
            inc.created_at = c.timestamp_str;
            inc.first_seen_epoch = epoch;
            inc.last_seen_epoch = epoch;
            inc.reported_type = c.reported_type;
            inc.repr_lat = c.latitude;
            inc.repr_lon = c.longitude;
            inc.call_ids.push_back(c.call_id);
            inc.call_count = 1;
            inc.active = true;
            incidents_map[key] = inc;
            active_keys.insert(key);
            seq_counter++;
            expiry_heap.push({epoch + TTL, seq_counter, key});
        }
    }

    cout << "Processing complete. New incidents: " << new_incident_count << ", duplicates merged: " << duplicate_count << "\n";
    cout << "Active incidents (current): " << active_keys.size() << "\n";

    // Export active incidents summary
    ofstream fout(outfile);
    if (!fout.is_open()) {
        cerr << "Failed to open output file " << outfile << "\n";
        return 1;
    }
    fout << "key,created_at,first_seen,last_seen,reported_type,repr_lat,repr_lon,call_count,call_ids\n";
    for (const auto &p : incidents_map) {
        const Incident &inc = p.second;
        // Only export active ones
        if (!inc.active) continue;
        // join call_ids with ';'
        string calls_joined;
        for (size_t i=0;i<inc.call_ids.size();++i) {
            if (i) calls_joined += ";";
            calls_joined += inc.call_ids[i];
        }
        // simple CSV escaping (if necessary)
        fout << "\"" << inc.key << "\"," << "\"" << inc.created_at << "\"," << inc.first_seen_epoch << "," << inc.last_seen_epoch << "," << inc.reported_type << "," << inc.repr_lat << "," << inc.repr_lon << "," << inc.call_count << "," << "\"" << calls_joined << "\"" << "\n";
    }
    fout.close();
    cout << "Wrote active incidents summary to " << outfile << "\n";

    // Print a few active incidents to console
    cout << "Sample active incidents:\n";
    int shown = 0;
    for (const auto &k : active_keys) {
        if (shown++ >= 10) break;
        auto &inc = incidents_map[k];
        cout << inc.key << " | calls=" << inc.call_count << " | repr=(" << inc.repr_lat << "," << inc.repr_lon << ") | first=" << inc.created_at << " | last=" << inc.last_seen_epoch << "\n";
    }

    return 0;
}
