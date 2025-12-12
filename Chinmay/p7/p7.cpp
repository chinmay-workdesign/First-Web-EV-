#include <bits/stdc++.h>
using namespace std;

// Rank intersections by vehicle counts using a stable Merge Sort.
// CSV input format expected:
// First line: literal header "num_intersections"
// Second line: a single integer N (number of intersections)
// Next N lines: id,count  (comma-separated, id is integer, count is integer)
// Example:
// num_intersections
// 3
// 0,150
// 1,200
// 2,150

void merge_sort_desc_stable(vector<pair<int,long long>>& a, int l, int r, vector<pair<int,long long>>& buf){
    if (r - l <= 1) return;
    int m = (l + r) >> 1;
    merge_sort_desc_stable(a, l, m, buf);
    merge_sort_desc_stable(a, m, r, buf);
    int i = l, j = m, k = l;
    while (i < m && j < r){
        // We want descending by count. For stability, when counts equal take left (i) first.
        if (a[i].second > a[j].second || (a[i].second == a[j].second)){
            buf[k++] = a[i++];
        } else {
            buf[k++] = a[j++];
        }
    }
    while (i < m) buf[k++] = a[i++];
    while (j < r) buf[k++] = a[j++];
    for (int t = l; t < r; ++t) a[t] = buf[t];
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string header;
    if (!getline(cin, header)) return 0;
    // expect header to be: num_intersections
    string line;
    if (!getline(cin, line)) return 0;
    int N = stoi(line);
    vector<pair<int,long long>> items;
    items.reserve(N);
    for (int i = 0; i < N; ++i){
        if (!getline(cin, line)) break;
        if (line.size() == 0){ --i; continue; }
        stringstream ss(line);
        int id; long long cnt; char c;
        ss >> id >> c >> cnt;
        items.push_back({id, cnt});
    }

    if ((int)items.size() != N) {
        cerr << "Warning: expected " << N << " items but read " << items.size() << "\n";
    }

    vector<pair<int,long long>> buf(max(1, (int)items.size()));
    merge_sort_desc_stable(items, 0, (int)items.size(), buf);

    // Output sorted list: id,count (descending by count, stable for equal counts)
    cout << "id,count\n";
    for (auto &p : items) cout << p.first << "," << p.second << "\n";

    return 0;
}
