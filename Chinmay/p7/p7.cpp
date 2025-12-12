#include <bits/stdc++.h>
using namespace std;

// Stable ranking of intersections by vehicle counts using stable Merge Sort
// Extended features:
// - Robust CSV parsing (comments '#' and blank lines allowed)
// - Command-line options: --input <file>, --output <file>, --top K, --generate-sample, --quiet
// - Stable sort descending by count; when counts equal, preserve input order (stability)
// - Optional secondary sort by intersection id while preserving stability when requested
// - Reports summary statistics (min, max, median, mean) and ties information
// - Can output ranks alongside id,count
//
// Expected CSV input format (simple):
// First non-comment line: literal header "num_intersections" (ignored but retained for compatibility)
// Next non-comment line: a single integer N (number of intersections)
// Next N non-comment lines: id,count  (comma-separated, id int, count integer)
// Example:
// num_intersections
// 3
// 0,150
// 1,200
// 2,150

static inline void ltrim(string &s){ s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch){ return !isspace(ch); })); }
static inline void rtrim(string &s){ s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch){ return !isspace(ch); }).base(), s.end()); }
static inline void trim(string &s){ ltrim(s); rtrim(s); }

vector<string> split_csv_line(const string &line){
    vector<string> tokens; string cur;
    for (size_t i = 0; i < line.size(); ++i){ char ch = line[i]; if (ch == ','){ string t = cur; trim(t); tokens.push_back(t); cur.clear(); } else cur.push_back(ch); }
    string t = cur; trim(t); if (!t.empty() || (line.size() && line.back()==',')) tokens.push_back(t); return tokens;
}

optional<long long> parse_int_safe(const string &s){ if (s.empty()) return nullopt; char *endptr = nullptr; errno = 0; long long val = strtoll(s.c_str(), &endptr, 10); if (errno != 0) return nullopt; while (*endptr){ if (!isspace((unsigned char)*endptr)) return nullopt; ++endptr; } return val; }

// Stable merge sort descending by count; when counts equal, preserve original index order (stability)
void merge_sort_desc_stable(vector<tuple<int,long long,int>> &a, int l, int r, vector<tuple<int,long long,int>> &buf){
    if (r - l <= 1) return;
    int m = (l + r) >> 1;
    merge_sort_desc_stable(a, l, m, buf);
    merge_sort_desc_stable(a, m, r, buf);
    int i = l, j = m, k = l;
    while (i < m && j < r){
        auto &L = a[i]; auto &R = a[j];
        long long lc = get<1>(L), rc = get<1>(R);
        if (lc > rc){ buf[k++] = a[i++]; }
        else if (lc < rc){ buf[k++] = a[j++]; }
        else {
            // counts equal: preserve stability by original index (smaller original_index comes first)
            int li = get<2>(L), ri = get<2>(R);
            if (li <= ri) buf[k++] = a[i++]; else buf[k++] = a[j++];
        }
    }
    while (i < m) buf[k++] = a[i++];
    while (j < r) buf[k++] = a[j++];
    for (int t = l; t < r; ++t) a[t] = buf[t];
}

// Secondary stable tie-breaker: sort by id but keep stability for equal (count,id)
void stable_secondary_by_id(vector<tuple<int,long long,int>> &a){
    // Perform stable partition: equal counts are in contiguous ranges; within each equal-count block, stable sort by id while preserving original order for equal ids
    int n = (int)a.size();
    int i = 0;
    while (i < n){
        int j = i+1;
        while (j < n && get<1>(a[j]) == get<1>(a[i])) ++j;
        // now [i,j) have equal counts; stable sort by id using stable_sort with a stable comparator
        stable_sort(a.begin()+i, a.begin()+j, [](const tuple<int,long long,int> &A, const tuple<int,long long,int> &B){ return get<0>(A) < get<0>(B); });
        i = j;
    }
}

string to_lower(const string &s){ string r = s; for (char &c : r) c = tolower((unsigned char)c); return r; }

bool write_sample_csv(const string &filename){
    ofstream ofs(filename); if (!ofs) return false;
    ofs << "num_intersections\n";
    ofs << "10\n";
    ofs << "0,150\n";
    ofs << "1,200\n";
    ofs << "2,150\n";
    ofs << "3,120\n";
    ofs << "4,200\n";
    ofs << "5,50\n";
    ofs << "6,200\n";
    ofs << "7,0\n";
    ofs << "8,150\n";
    ofs << "9,50\n";
    ofs.close(); return true;
}

int main(int argc, char **argv){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string input_filename, output_filename;
    bool generate_sample = false;
    bool quiet = false;
    int top_k = -1; // -1 means output all
    bool secondary_by_id = false;

    for (int i = 1; i < argc; ++i){ string a = argv[i]; if (a == "--input" && i+1 < argc) input_filename = argv[++i]; else if (a=="--output" && i+1 < argc) output_filename = argv[++i]; else if (a=="--generate-sample") generate_sample = true; else if (a=="--quiet") quiet = true; else if (a=="--top" && i+1 < argc) top_k = stoi(argv[++i]); else if (a=="--secondary-by-id") secondary_by_id = true; else { cerr << "Unknown arg: " << a << "\n"; cerr << "Usage: " << argv[0] << " [--input file.csv] [--output file.txt] [--generate-sample] [--top K] [--secondary-by-id] [--quiet]\n"; return 1; } }

    if (generate_sample){ const string name = input_filename.empty() ? string("sample_intersections.csv") : input_filename; if (write_sample_csv(name)){ cout << "Wrote sample CSV to: " << name << "\n"; if (input_filename.empty()) cout << "Use --input " << name << " to run the program.\n"; return 0; } else { cerr << "Failed to write sample CSV to: " << name << "\n"; return 2; } }

    istream *inptr = &cin; ifstream ifs; if (!input_filename.empty()){ ifs.open(input_filename); if (!ifs){ cerr << "Failed to open input file: " << input_filename << "\n"; return 3; } inptr = &ifs; }

    // Read first non-empty non-comment line as header (expected literal "num_intersections" but we accept anything)
    string header; while (true){ if (!getline(*inptr, header)){ cerr << "No input provided. Use --generate-sample to create one.\n"; return 4; } trim(header); if (header.empty()) continue; if (header[0] == '#') continue; break; }

    // Read next non-empty non-comment line for N
    string nline; while (true){ if (!getline(*inptr, nline)){ cerr << "Missing N line (number of intersections).\n"; return 5; } trim(nline); if (nline.empty()) continue; if (nline[0] == '#') continue; break; }
    auto n_opt = parse_int_safe(nline);
    if (!n_opt){ cerr << "Could not parse N from line: '" << nline << "'\n"; return 6; }
    int N = (int)*n_opt;
    if (N < 0){ cerr << "Invalid N: " << N << "\n"; return 7; }

    vector<tuple<int,long long,int>> items; items.reserve(max(0,N));
    string line;
    int read_items = 0;
    int original_index = 0;

    while (read_items < N && getline(*inptr, line)){
        trim(line);
        if (line.empty()) continue;
        if (line[0] == '#') continue;
        auto parts = split_csv_line(line);
        if (parts.size() < 2){ if (!quiet) cerr << "Skipping invalid item line: '" << line << "'\n"; continue; }
        auto id_opt = parse_int_safe(parts[0]); auto cnt_opt = parse_int_safe(parts[1]);
        if (!id_opt || !cnt_opt){ if (!quiet) cerr << "Skipping line with non-integers: '" << line << "'\n"; continue; }
        int id = (int)*id_opt; long long cnt = *cnt_opt;
        if (id < 0) { if (!quiet) cerr << "Skipping negative id: " << id << "\n"; continue; }
        items.emplace_back(id, cnt, original_index++);
        ++read_items;
    }

    if (read_items != N) {
        if (!quiet) cerr << "Warning: expected " << N << " items but read " << read_items << ".\n";
    }

    // sort
    vector<tuple<int,long long,int>> buf(max(1,(int)items.size()));
    merge_sort_desc_stable(items, 0, (int)items.size(), buf);
    if (secondary_by_id) stable_secondary_by_id(items);

    // compute statistics
    vector<long long> counts;
    counts.reserve(items.size());
    for (auto &t : items) counts.push_back(get<1>(t));
    long long total = 0; for (auto v : counts) total += v;
    double mean = counts.empty() ? 0.0 : (double)total / counts.size();
    long long mn = counts.empty() ? 0 : *min_element(counts.begin(), counts.end());
    long long mx = counts.empty() ? 0 : *max_element(counts.begin(), counts.end());
    double median = 0.0;
    if (!counts.empty()){
        vector<long long> tmp = counts; sort(tmp.begin(), tmp.end());
        int sz = (int)tmp.size();
        if (sz % 2 == 1) median = tmp[sz/2]; else median = (tmp[sz/2-1] + tmp[sz/2]) / 2.0;
    }

    // detect ties: groups with equal counts
    vector<pair<long long, int>> tie_groups; // (count, size)
    for (size_t i = 0; i < items.size();){
        size_t j = i+1; while (j < items.size() && get<1>(items[j]) == get<1>(items[i])) ++j;
        if (j - i > 1) tie_groups.emplace_back(get<1>(items[i]), (int)(j-i));
        i = j;
    }

    // prepare output lines
    vector<string> out_lines;
    out_lines.reserve(items.size()+10);
    out_lines.push_back(string("id,count"));
    int limit = (top_k <= 0 || top_k > (int)items.size()) ? (int)items.size() : top_k;
    for (int i = 0; i < limit; ++i){ out_lines.push_back( to_string(get<0>(items[i])) + "," + to_string(get<1>(items[i])) ); }

    // also produce ranked output if requested
    vector<string> rank_lines;
    rank_lines.push_back(string("rank,id,count"));
    for (int i = 0; i < limit; ++i){ rank_lines.push_back( to_string(i+1) + "," + to_string(get<0>(items[i])) + "," + to_string(get<1>(items[i])) ); }

    ostringstream summary;
    summary << "Stable Rank Report\n";
    summary << "Total read: " << items.size() << " intersections (declared N=" << N << ")\n";
    summary << "Min count: " << mn << ", Max count: " << mx << ", Mean: " << fixed << setprecision(2) << mean << ", Median: " << median << "\n";
    summary << "Tie groups: " << tie_groups.size() << "\n";
    for (auto &tg : tie_groups) summary << "  count=" << tg.first << " size=" << tg.second << "\n";
    summary << "Top output limit: " << limit << "\n";

    // write output either to file or stdout
    if (!output_filename.empty()){
        ofstream ofs(output_filename);
        if (!ofs){ cerr << "Failed to open output file: " << output_filename << "\n"; return 8; }
        for (auto &ln : out_lines) ofs << ln << "\n";
        ofs << "\n";
        for (auto &ln : rank_lines) ofs << ln << "\n";
        ofs << "\n" << summary.str();
        ofs.close(); if (!quiet) cout << "Wrote sorted output and summary to: " << output_filename << "\n";
    } else {
        for (auto &ln : out_lines) cout << ln << "\n";
        cout << "\n";
        for (auto &ln : rank_lines) cout << ln << "\n";
        cout << "\n" << summary.str();
    }

    return 0;
}

/*
SAMPLE CSV (suitable changes):

The program expects:
- a header line (literal string like "num_intersections"),
- a single line with N,
- then N lines of id,count

Example sample (already in this file's --generate-sample output):

num_intersections
10
0,150
1,200
2,150
3,120
4,200
5,50
6,200
7,0
8,150
9,50

*/
