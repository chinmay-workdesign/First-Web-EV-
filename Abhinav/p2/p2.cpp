// tsp_dp.cpp
// Compile: g++ -std=c++17 -O2 -o tsp_dp tsp_dp.cpp
// Usage: ./tsp_dp /mnt/data/dumpsters.csv [K] [base_x] [base_y]
// Example: ./tsp_dp /mnt/data/dumpsters.csv 16 5000 5000

#include <bits/stdc++.h>
using namespace std;
struct Dump { string id; double x,y; };
double euclid(double x1,double y1,double x2,double y2){
    double dx=x1-x2, dy=y1-y2; return sqrt(dx*dx+dy*dy);
}

int main(int argc, char** argv){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    if(argc < 2){
        cerr<<"Usage: "<<argv[0]<<" dumpsters.csv [K=16] [base_x=5000] [base_y=5000]\n";
        return 1;
    }
    string csv = argv[1];
    int K = 16;
    double base_x = 5000.0, base_y = 5000.0;
    if(argc >= 3) K = stoi(argv[2]);
    if(argc >= 5){ base_x = stod(argv[3]); base_y = stod(argv[4]); }

    // Read all dumpsters
    vector<Dump> all;
    {
        ifstream fin(csv);
        if(!fin){ cerr<<"Cannot open "<<csv<<"\n"; return 1; }
        string header; getline(fin, header);
        string line;
        while(getline(fin,line)){
            if(line.empty()) continue;
            stringstream ss(line);
            string id, xs, ys;
            getline(ss,id,',');
            getline(ss,xs,',');
            getline(ss,ys,',');
            Dump d; d.id=id; d.x=stod(xs); d.y=stod(ys);
            all.push_back(d);
        }
    }
    if(all.empty()){ cerr<<"No dumpsters loaded\n"; return 1; }
    cout<<"Loaded "<<all.size()<<" dumpsters.\n";

    // Select K nearest to base
    int N_all = (int)all.size();
    vector<pair<double,int>> dist_idx;
    dist_idx.reserve(N_all);
    for(int i=0;i<N_all;i++){
        double d = euclid(base_x, base_y, all[i].x, all[i].y);
        dist_idx.push_back({d,i});
    }
    sort(dist_idx.begin(), dist_idx.end());
    if(K > N_all) K = N_all;
    vector<Dump> nodes; nodes.reserve(K+1);
    // Node 0 is base
    Dump base; base.id = "BASE"; base.x = base_x; base.y = base_y;
    nodes.push_back(base);
    for(int i=0;i<K;i++){
        nodes.push_back(all[dist_idx[i].second]);
    }
    cout<<"Solving exact TSP for K="<<K<<" nearest dumpsters (node count incl. base = "<<K+1<<")\n";

    // Build distance matrix (size K+1)
    int M = K+1;
    vector<vector<double>> dist(M, vector<double>(M,0.0));
    for(int i=0;i<M;i++) for(int j=0;j<M;j++)
        dist[i][j] = euclid(nodes[i].x, nodes[i].y, nodes[j].x, nodes[j].y);

    // DP: masks over K dumpsters (bits 0..K-1 correspond to nodes 1..K)
    int FULL = 1<<K;
    const double INF = 1e18;
    // dp[mask][last] where last in 1..K; store parent for reconstruction
    vector<vector<double>> dp(FULL, vector<double>(K+1, INF));
    vector<vector<int>> parent(FULL, vector<int>(K+1, -1));

    // Initialize
    for(int i=1;i<=K;i++){
        int mask = 1<<(i-1);
        dp[mask][i] = dist[0][i]; // base -> i
        parent[mask][i] = 0;
    }

    // Iterate masks
    for(int mask=0; mask<FULL; ++mask){
        for(int last=1; last<=K; ++last){
            if(!(mask & (1<<(last-1)))) continue;
            double cur = dp[mask][last];
            if(cur >= INF) continue;
            // Try to go to next
            for(int nxt=1; nxt<=K; ++nxt){
                if(mask & (1<<(nxt-1))) continue;
                int nmask = mask | (1<<(nxt-1));
                double cand = cur + dist[last][nxt];
                if(cand < dp[nmask][nxt]){
                    dp[nmask][nxt] = cand;
                    parent[nmask][nxt] = last;
                }
            }
        }
    }

    // Close tour: full mask
    int fullmask = FULL - 1;
    double best = INF; int lastBest = -1;
    for(int last=1; last<=K; ++last){
        double cand = dp[fullmask][last] + dist[last][0];
        if(cand < best){ best = cand; lastBest = last; }
    }
    if(best >= INF){ cerr<<"No tour found\n"; return 1; }

    // Reconstruct path
    vector<int> pathNodes; // indices into nodes (0..K), start at base, visit in order, return base
    int curMask = fullmask;
    int curLast = lastBest;
    while(curLast != 0){
        pathNodes.push_back(curLast);
        int p = parent[curMask][curLast];
        curMask ^= 1<<(curLast-1);
        curLast = p;
    }
    reverse(pathNodes.begin(), pathNodes.end());
    // Output full route including base at start and end
    cout<<"Optimal tour cost (approx): "<<fixed<<setprecision(6)<<best<<"\n";
    cout<<"Route: BASE -> ";
    for(int idx : pathNodes) cout<<nodes[idx].id<<" -> ";
    cout<<"BASE\n";

    // Write route.csv
    ofstream fout("route.csv");
    fout<<"sequence,site_id,x,y\n";
    fout<<0<<","<<nodes[0].id<<","<<nodes[0].x<<","<<nodes[0].y<<"\n";
    int seq = 1;
    for(int idx : pathNodes){
        fout<<seq++<<","<<nodes[idx].id<<","<<nodes[idx].x<<","<<nodes[idx].y<<"\n";
    }
    fout<<seq<<","<<nodes[0].id<<","<<nodes[0].x<<","<<nodes[0].y<<"\n";
    fout.close();
    cout<<"Wrote route.csv with sequence (BASE start and end).\n";

    return 0;
}
