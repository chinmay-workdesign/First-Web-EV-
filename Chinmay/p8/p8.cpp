#include <bits/stdc++.h>
using namespace std;

// A* path planning in a 3D grid.
// CSV input format:
// Header line: grid_x,grid_y,grid_z,start_x,start_y,start_z,goal_x,goal_y,goal_z
// Second line: X,Y,Z,SX,SY,SZ,GX,GY,GZ
// Next lines: x,y,z,blocked (blocked = 1 if obstacle, 0 otherwise)

struct Node {
    int x, y, z;
    double f;
};

struct Cmp {
    bool operator()(const Node& a, const Node& b) const {
        return a.f > b.f;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string header;
    if (!getline(cin, header)) return 0;
    string line;
    if (!getline(cin, line)) return 0;

    stringstream ss(line);
    int X,Y,Z,sx,sy,sz,gx,gy,gz; char c;
    ss >> X >> c >> Y >> c >> Z >> c >> sx >> c >> sy >> c >> sz >> c >> gx >> c >> gy >> c >> gz;

    vector<vector<vector<int>>> grid(X, vector<vector<int>>(Y, vector<int>(Z, 0)));
    while (getline(cin, line)){
        if (line.size()==0) continue;
        stringstream es(line);
        int x,y,z,b;
        es >> x >> c >> y >> c >> z >> c >> b;
        if (x>=0&&x<X&&y>=0&&y<Y&&z>=0&&z<Z)
            grid[x][y][z] = b;
    }

    const double INF = 1e18;
    vector<vector<vector<double>>> dist(X, vector<vector<double>>(Y, vector<double>(Z, INF)));
    priority_queue<Node, vector<Node>, Cmp> pq;

    auto heuristic = [&](int x,int y,int z){
        return sqrt((x-gx)*(x-gx)+(y-gy)*(y-gy)+(z-gz)*(z-gz));
    };

    dist[sx][sy][sz] = 0;
    pq.push({sx,sy,sz,heuristic(sx,sy,sz)});

    int dx[6]={1,-1,0,0,0,0};
    int dy[6]={0,0,1,-1,0,0};
    int dz[6]={0,0,0,0,1,-1};

    while(!pq.empty()){
        Node cur = pq.top(); pq.pop();
        if (cur.x==gx && cur.y==gy && cur.z==gz) break;
        for(int i=0;i<6;i++){
            int nx=cur.x+dx[i], ny=cur.y+dy[i], nz=cur.z+dz[i];
            if(nx<0||ny<0||nz<0||nx>=X||ny>=Y||nz>=Z) continue;
            if(grid[nx][ny][nz]) continue;
            double nd = dist[cur.x][cur.y][cur.z] + 1.0;
            if(nd < dist[nx][ny][nz]){
                dist[nx][ny][nz] = nd;
                pq.push({nx,ny,nz, nd + heuristic(nx,ny,nz)});
            }
        }
    }

    if (dist[gx][gy][gz] == INF) cout << "NO_PATH\n";
    else cout << "PATH_LENGTH," << dist[gx][gy][gz] << '\n';

    return 0;
}
