#include <iostream>
#include <vector>
#include <algorithm>
#include <set>
#include <stack>
#include <unordered_map>
using namespace std;

/*
MODEL 10: SULCD-RE
Land-Use Conflict Detection using
Segment Logic + Sweep Line + Union-Find
*/

// ---------------- UNION-FIND ----------------
struct DSU {
    vector<int> parent, rankv;
    DSU(int n) {
        parent.resize(n);
        rankv.resize(n,0);
        for(int i=0;i<n;i++) parent[i]=i;
    }
    int find(int x) {
        if(parent[x]==x) return x;
        return parent[x]=find(parent[x]);
    }
    void unite(int a,int b) {
        a=find(a); b=find(b);
        if(a!=b) {
            if(rankv[a]<rankv[b]) swap(a,b);
            parent[b]=a;
            if(rankv[a]==rankv[b]) rankv[a]++;
        }
    }
};

// ---------------- INTERVAL STRUCT ----------------
struct Interval {
    int l, r, id;
};

// ---------------- MAIN ----------------
int main() {

    cout << "=== SULCD-RE : Land-Use Conflict Detection ===\n";

    // ---------- ZONE TYPES ----------
    unordered_map<int,string> zoneType = {
        {0,"Residential"},
        {1,"Industrial"},
        {2,"School"},
        {3,"Highway"},
        {4,"Commercial"}
    };

    // ---------- INTERVALS ----------
    vector<Interval> zones = {
        {1,5,0},
        {4,8,1},
        {10,14,2},
        {13,16,3},
        {6,9,4}
    };

    // ---------- SORT BY START (SWEEP LINE) ----------
    sort(zones.begin(), zones.end(),
         [](Interval a, Interval b){ return a.l < b.l; });

    set<pair<int,int>> active; // (end, id)
    stack<pair<int,int>> conflictStack;

    DSU dsu(zones.size());

    cout << "\nDetected Conflicts:\n";

    for(auto &z : zones) {
        while(!active.empty() && active.begin()->first < z.l)
            active.erase(active.begin());

        for(auto &a : active) {
            int other = a.second;
            cout << "Conflict: Zone " << z.id
                 << " (" << zoneType[z.id] << ") with Zone "
                 << other << " (" << zoneType[other] << ")\n";

            dsu.unite(z.id, other);
            conflictStack.push({z.id, other});
        }

        active.insert({z.r, z.id});
    }

    // ---------- CONFLICT GROUPS ----------
    unordered_map<int, vector<int>> groups;
    for(auto &z : zones)
        groups[dsu.find(z.id)].push_back(z.id);

    cout << "\nConflict Groups:\n";
    for(auto &g : groups) {
        cout << "Group: ";
        for(int id : g.second)
            cout << id << " ";
        cout << endl;
    }

    // ---------- STACK TRACE ----------
    cout << "\nConflict Resolution Trace (Stack):\n";
    while(!conflictStack.empty()) {
        auto p = conflictStack.top();
        conflictStack.pop();
        cout << "Review conflict between Zone "
             << p.first << " and Zone " << p.second << endl;
    }

    cout << "\nLand-use conflict detection completed.\n";
    return 0;
}
