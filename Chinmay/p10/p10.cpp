#include <bits/stdc++.h>
using namespace std;

// Greedy Set Cover approximation
// CSV format expected:
// Header line: num_locations,num_segments,budget
// Second line: N,M,B
// Next N lines: loc_id,seg1|seg2|seg3|...  (segments are integers in [0,M-1], '|' separated)
// Example:
// num_locations,num_segments,budget
// 1000,2000,50
// 0,1|5|7|42
// 1,2|3|7

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string header;
    if (!getline(cin, header)) return 0;
    string line;
    if (!getline(cin, line)) return 0;
    stringstream s2(line);
    int N, M, B; char c;
    s2 >> N >> c >> M >> c >> B;

    vector<vector<int>> covers(N);
    for (int i = 0; i < N; ++i){
        if (!getline(cin, line)) break;
        if (line.size() == 0){ --i; continue; }
        auto pos = line.find(',');
        if (pos == string::npos) continue;
        int loc = stoi(line.substr(0,pos));
        string rest = line.substr(pos+1);
        stringstream ss(rest);
        string token;
        while (getline(ss, token, '|')){
            if (token.size()==0) continue;
            int seg = stoi(token);
            if (seg >=0 && seg < M) covers[loc].push_back(seg);
        }
        // remove duplicates inside a location
        sort(covers[loc].begin(), covers[loc].end());
        covers[loc].erase(unique(covers[loc].begin(), covers[loc].end()), covers[loc].end());
    }

    vector<char> covered(M, false);
    int covered_count = 0;
    vector<int> chosen;
    chosen.reserve(B);

    // max-heap of (estimated_new_covered, loc)
    priority_queue<pair<int,int>> pq;
    for (int i = 0; i < N; ++i) pq.push({(int)covers[i].size(), i});

    auto true_gain = [&](int loc)->int{
        int g=0;
        for (int s: covers[loc]) if (!covered[s]) ++g;
        return g;
    };

    while ((int)chosen.size() < B && covered_count < M && !pq.empty()){
        auto [est, loc] = pq.top(); pq.pop();
        int tg = true_gain(loc);
        if (tg == 0) continue; // nothing new
        // lazy update: if estimate mismatches, push updated
        if (tg != est){
            pq.push({tg, loc});
            continue;
        }
        // choose this location
        chosen.push_back(loc);
        for (int s: covers[loc]){
            if (!covered[s]){ covered[s]=1; ++covered_count; }
        }
    }

    // Output results
    cout << "ChosenLocationsCount," << chosen.size() << "\n";
    cout << "TotalSegments," << M << "\n";
    cout << "CoveredSegments," << covered_count << "\n";
    cout << "ChosenLocations (one per line):\n";
    for (int x: chosen) cout << x << "\n";
    return 0;
}
