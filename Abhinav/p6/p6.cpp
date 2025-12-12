// interval_scheduling_or.cpp
// Interval Scheduling (Greedy) for scheduling maximum number of surgeries in one OR.
//
// Compile: g++ -std=c++17 -O2 -o interval_scheduling_or interval_scheduling_or.cpp
//
// Usage: ./interval_scheduling_or surgeries.csv [--min-duration M] [--max-duration M]
//        [--output out.csv] [--verbose]
//
// Input CSV must have header including: request_id,start,end,duration_minutes
// where start/end are ISO datetimes like "2025-12-15 08:30:00" or "2025-12-15 08:30"
//
// Output: scheduled_surgeries.csv (by default) listing selected intervals in chronological order.

#include <bits/stdc++.h>
using namespace std;

// ----------------------- DateTime parsing helpers --------------------------

// Parse simple ISO-like datetime strings "YYYY-MM-DD HH:MM[:SS]" into time_t (seconds since epoch).
// This function does not handle time zones — assumes local or naive timestamps; same semantics used across inputs.
bool parse_iso_datetime(const string &s, time_t &out_t) {
    // Accept formats: "YYYY-MM-DD HH:MM" or "YYYY-MM-DD HH:MM:SS"
    // We'll parse numbers manually
    string t = s;
    // replace 'T' with space if present
    for (char &c : t) if (c == 'T') c = ' ';
    // trim
    auto trim = [](string &str){
        const char* ws = " \t\r\n";
        size_t a = str.find_first_not_of(ws);
        if (a == string::npos) { str.clear(); return; }
        size_t b = str.find_last_not_of(ws);
        str = str.substr(a, b - a + 1);
    };
    trim(t);
    if (t.empty()) return false;
    // split date and time
    size_t sp = t.find(' ');
    if (sp == string::npos) return false;
    string date = t.substr(0, sp);
    string time = t.substr(sp+1);
    vector<int> dparts;
    vector<int> tparts;
    // parse date
    {
        string cur;
        for (char c : date) {
            if (c == '-') { if (cur.empty()) return false; dparts.push_back(stoi(cur)); cur.clear(); }
            else cur.push_back(c);
        }
        if (!cur.empty()) dparts.push_back(stoi(cur));
    }
    // parse time
    {
        string cur;
        for (char c : time) {
            if (c == ':') { if (cur.empty()) return false; tparts.push_back(stoi(cur)); cur.clear(); }
            else if (isdigit((unsigned char)c)) cur.push_back(c);
            else break; // ignore extra
        }
        if (!cur.empty()) tparts.push_back(stoi(cur));
    }
    if (dparts.size() != 3 || tparts.size() < 2) return false;
    int year = dparts[0], month = dparts[1], day = dparts[2];
    int hour = tparts[0], minute = tparts[1], second = (tparts.size() >= 3 ? tparts[2] : 0);
    // Validate ranges roughly
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > 31) return false;
    if (hour < 0 || hour > 23) return false;
    if (minute < 0 || minute > 59) return false;
    if (second < 0 || second > 59) return false;

    // Use tm struct and std::mktime (which assumes local time)
    struct tm tm_time;
    tm_time.tm_year = year - 1900;
    tm_time.tm_mon = month - 1;
    tm_time.tm_mday = day;
    tm_time.tm_hour = hour;
    tm_time.tm_min = minute;
    tm_time.tm_sec = second;
    tm_time.tm_isdst = -1; // let mktime determine DST
    time_t epoch = mktime(&tm_time);
    if (epoch == (time_t)-1) return false;
    out_t = epoch;
    return true;
}

// ---------------------------- Interval struct ------------------------------

struct Interval {
    string id;
    time_t start; // seconds since epoch
    time_t end;
    int duration_minutes;
    // optional: priority or weight
    double weight = 1.0;
};

// -------------------------- CSV Parsing -----------------------------------

bool parse_csv_line(const string &line, vector<string> &out) {
    out.clear();
    string cur;
    int commas_seen = 0;
    // For this CSV we allow commas in address fields but for surgeries we expect basic fields.
    // We'll split on commas simply.
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == ',') {
            out.push_back(cur);
            cur.clear();
            commas_seen++;
            continue;
        }
        cur.push_back(c);
    }
    out.push_back(cur);
    // trim spaces
    for (auto &s : out) {
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == string::npos) { s.clear(); continue; }
        size_t b = s.find_last_not_of(" \t\r\n");
        s = s.substr(a, b - a + 1);
    }
    return true;
}

bool load_intervals_from_csv(const string &filename, vector<Interval> &out, string &err) {
    ifstream fin(filename);
    if (!fin.is_open()) { err = "Cannot open file: " + filename; return false; }
    string header;
    if (!getline(fin, header)) { err = "Empty file"; return false; }
    vector<string> cols;
    parse_csv_line(header, cols);
    // Map columns to indices we need
    // expected: request_id,start,end,duration_minutes (in any order ideally)
    unordered_map<string,int> col_index;
    for (int i=0;i<(int)cols.size();++i) {
        string c = cols[i];
        for (auto &ch : c) ch = (char)tolower((unsigned char)ch);
        // trim
        size_t a = c.find_first_not_of(" \t\r\n");
        if (a == string::npos) continue;
        size_t b = c.find_last_not_of(" \t\r\n");
        c = c.substr(a, b-a+1);
        col_index[c] = i;
    }
    // find needed columns
    auto find_col = [&](vector<string> candidates)->int {
        for (auto &cand : candidates) {
            string key = cand;
            for (auto &ch : key) ch = (char)tolower((unsigned char)ch);
            if (col_index.find(key) != col_index.end()) return col_index[key];
        }
        return -1;
    };
    int id_col = find_col({"request_id","id","req_id","requestid"});
    int start_col = find_col({"start","start_time","starttime","begin"});
    int end_col = find_col({"end","end_time","endtime","finish"});
    int dur_col = find_col({"duration_minutes","duration","duration_min","minutes"});
    if (id_col == -1 || start_col == -1 || end_col == -1) {
        err = "CSV header must include request_id, start, and end columns (names may vary). Found columns: ";
        for (auto &p : col_index) err += p.first + " ";
        return false;
    }
    string line;
    int line_no = 1;
    while (getline(fin, line)) {
        ++line_no;
        if (line.empty()) continue;
        vector<string> fields;
        parse_csv_line(line, fields);
        // tolerate missing columns gracefully
        string id = (id_col < (int)fields.size()) ? fields[id_col] : ("row" + to_string(line_no));
        string start_s = (start_col < (int)fields.size()) ? fields[start_col] : "";
        string end_s = (end_col < (int)fields.size()) ? fields[end_col] : "";
        string dur_s = (dur_col < (int)fields.size()) ? fields[dur_col] : "";
        time_t ts, te;
        if (!parse_iso_datetime(start_s, ts)) {
            cerr << "Warning: failed to parse start time on line " << line_no << ": '" << start_s << "'. Skipping.\n";
            continue;
        }
        if (!parse_iso_datetime(end_s, te)) {
            cerr << "Warning: failed to parse end time on line " << line_no << ": '" << end_s << "'. Skipping.\n";
            continue;
        }
        int dur = 0;
        if (!dur_s.empty()) {
            try { dur = stoi(dur_s); } catch(...) { dur = (int)((te - ts)/60); }
        } else dur = (int)((te - ts)/60);
        if (dur <= 0) {
            cerr << "Warning: non-positive duration at line " << line_no << ". Skipping.\n";
            continue;
        }
        Interval iv;
        iv.id = id;
        iv.start = ts;
        iv.end = te;
        iv.duration_minutes = dur;
        out.push_back(iv);
    }
    fin.close();
    return true;
}

// --------------------------- Greedy scheduling -----------------------------

vector<Interval> schedule_max_nonoverlapping(vector<Interval> intervals) {
    // sort by end time (earliest end first). Tie-breaker: earlier start
    sort(intervals.begin(), intervals.end(), [](const Interval &a, const Interval &b) {
        if (a.end != b.end) return a.end < b.end;
        return a.start < b.start;
    });
    vector<Interval> chosen;
    time_t last_end = numeric_limits<time_t>::min();
    for (const auto &iv : intervals) {
        if (iv.start >= last_end) {
            chosen.push_back(iv);
            last_end = iv.end;
        }
    }
    return chosen;
}

// ----------------------------- Utilities ----------------------------------

string time_t_to_iso(time_t t) {
    struct tm tm_time;
    localtime_r(&t, &tm_time);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_time);
    return string(buf);
}

// ----------------------------- Main ---------------------------------------

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " surgeries.csv [--min-duration M] [--max-duration M] [--output out.csv] [--verbose]\n";
        return 1;
    }
    string infile = argv[1];
    int min_dur = 0, max_dur = 1000000;
    string outfile = "scheduled_surgeries.csv";
    bool verbose = false;
    for (int i=2;i<argc;++i) {
        string s = argv[i];
        if (s == "--min-duration" && i+1<argc) { min_dur = stoi(argv[++i]); }
        else if (s == "--max-duration" && i+1<argc) { max_dur = stoi(argv[++i]); }
        else if (s == "--output" && i+1<argc) { outfile = argv[++i]; }
        else if (s == "--verbose") verbose = true;
    }

    vector<Interval> intervals;
    string err;
    if (!load_intervals_from_csv(infile, intervals, err)) {
        cerr << "Error loading CSV: " << err << "\n";
        return 1;
    }
    if (intervals.empty()) {
        cerr << "No valid intervals loaded.\n";
        return 1;
    }
    cout << "Loaded " << intervals.size() << " surgery requests from " << infile << "\n";

    // apply duration filters
    vector<Interval> filtered;
    filtered.reserve(intervals.size());
    for (auto &iv : intervals) {
        if (iv.duration_minutes < min_dur) continue;
        if (iv.duration_minutes > max_dur) continue;
        filtered.push_back(iv);
    }
    cout << "After duration filter: " << filtered.size() << " intervals remain.\n";

    // Run greedy scheduling
    auto scheduled = schedule_max_nonoverlapping(filtered);
    cout << "Scheduled " << scheduled.size() << " surgeries (maximum by greedy algorithm).\n";

    // Sort scheduled by start time for human-friendly output
    sort(scheduled.begin(), scheduled.end(), [](const Interval &a, const Interval &b) {
        if (a.start != b.start) return a.start < b.start;
        return a.end < b.end;
    });

    // Write to CSV
    ofstream fout(outfile);
    if (!fout.is_open()) {
        cerr << "Failed to open output file: " << outfile << "\n";
        return 1;
    }
    fout << "request_id,start,end,duration_minutes\n";
    for (auto &iv : scheduled) {
        fout << iv.id << "," << time_t_to_iso(iv.start) << "," << time_t_to_iso(iv.end) << "," << iv.duration_minutes << "\n";
    }
    fout.close();
    cout << "Wrote scheduled surgeries to " << outfile << "\n";

    if (verbose) {
        cout << "Full scheduled list:\n";
        for (auto &iv : scheduled) {
            cout << iv.id << " | " << time_t_to_iso(iv.start) << " -> " << time_t_to_iso(iv.end) << " | " << iv.duration_minutes << "min\n";
        }
    }

    // Summary statistics
    if (!scheduled.empty()) {
        time_t first_start = scheduled.front().start;
        time_t last_end = scheduled.back().end;
        double total_scheduled_minutes = 0.0;
        for (auto &iv : scheduled) total_scheduled_minutes += iv.duration_minutes;
        cout << fixed << setprecision(1);
        cout << "First scheduled start: " << time_t_to_iso(first_start) << "\n";
        cout << "Last scheduled end:   " << time_t_to_iso(last_end) << "\n";
        cout << "Total scheduled surgery time: " << total_scheduled_minutes << " minutes (~" << (total_scheduled_minutes/60.0) << " hours)\n";
    }

    return 0;
}
