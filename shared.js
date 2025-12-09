// --- Theme Toggle Logic ---
function toggleTheme() {
    const html = document.documentElement;
    const icon = document.getElementById('theme-icon');
    const mobileIcon = document.getElementById('mobile-theme-icon');
    
    if (html.classList.contains('dark')) {
        html.classList.remove('dark');
        localStorage.setItem('theme', 'light');
        if (icon) {
            icon.classList.remove('fa-sun');
            icon.classList.add('fa-moon');
            icon.classList.remove('text-yellow-500');
            icon.classList.add('text-gray-600');
        }
        
        if (mobileIcon) {
            mobileIcon.classList.remove('fa-sun');
            mobileIcon.classList.add('fa-moon');
            mobileIcon.classList.remove('text-yellow-500');
            mobileIcon.classList.add('text-gray-600');
        }
    } else {
        html.classList.add('dark');
        localStorage.setItem('theme', 'dark');
        if (icon) {
            icon.classList.remove('fa-moon');
            icon.classList.add('fa-sun');
            icon.classList.add('text-yellow-500');
            icon.classList.remove('text-gray-600');
        }
        
        if (mobileIcon) {
            mobileIcon.classList.remove('fa-moon');
            mobileIcon.classList.add('fa-sun');
            mobileIcon.classList.add('text-yellow-500');
            mobileIcon.classList.remove('text-gray-600');
        }
    }
}

// Initialize Theme
if (localStorage.theme === 'light' || (!('theme' in localStorage) && window.matchMedia('(prefers-color-scheme: light)').matches)) {
     document.documentElement.classList.remove('dark');
     const icon = document.getElementById('theme-icon');
     if (icon) {
         icon.classList.remove('fa-sun', 'text-yellow-500');
         icon.classList.add('fa-moon', 'text-gray-600');
     }
     const mobileIcon = document.getElementById('mobile-theme-icon');
     if (mobileIcon) {
         mobileIcon.classList.remove('fa-sun', 'text-yellow-500');
         mobileIcon.classList.add('fa-moon', 'text-gray-600');
     }
} else {
    document.documentElement.classList.add('dark');
}

function toggleMobileMenu() {
    const menu = document.getElementById('mobile-menu');
    if (menu) {
        menu.classList.toggle('hidden');
    }
}

// --- Modal Logic ---
function openProblemModal(element) {
    const title = element.querySelector('h4').innerText;
    const data = problemDatabase[title];
    
    if (data) {
        document.getElementById('modalTitle').innerText = title;
        document.getElementById('modalProblem').innerText = data.problem;
        document.getElementById('modalSolution').innerText = data.solution;
        
        const codeElement = document.getElementById('modalCode');
        codeElement.textContent = data.code;
        delete codeElement.dataset.highlighted;
        hljs.highlightElement(codeElement);
        
        document.getElementById('problemModal').classList.add('active');
    } else {
        console.error("No data found for: " + title);
    }
}

function closeProblemModal() {
    document.getElementById('problemModal').classList.remove('active');
}

document.addEventListener('keydown', function(event) {
    if (event.key === "Escape") {
        closeProblemModal();
    }
});

// --- Problem Data Database ---
const problemDatabase = {
    // --- Chinmay's Problems ---
    "Shortest Path Navigation": {
        problem: "Electric Vehicles (EVs) suffer from range anxiety. In a city with dynamic traffic conditions, finding the shortest path based on distance alone is insufficient. We need to find the quickest path from a start point to an end point in a city graph where edge weights represent real-time traffic delay, ensuring optimal battery usage and time efficiency.",
        solution: "We model the city as a weighted directed graph where intersections are nodes and roads are edges. Dijkstra's Algorithm is the optimal choice here because edge weights (time delay) are non-negative. We use a Priority Queue to always explore the shortest known path first, expanding layer by layer. This guarantees finding the shortest path with a time complexity of O(E log V).",
        code: `#include<iostream>
#include<algorithm>
using namespace std;

class dijkstra {
public:
    int path[4];
    int dist[4];
    int visited[4];
    void initialise(int cost[4][4], int source);
    int minimum();
    void updation(int cost[4][4]);
    void shortest_path(int cost[4][4],int source);
    void print(int source);
};

void dijkstra::initialise(int cost[4][4],int source) {
    for(int i=0; i<4; i++) {
        path[i] = source;
        dist[i] = (cost[source][i] == 0) ? 10000000 : cost[source][i];
        visited[i] = 0;
    }
    visited[source] = 1;
    dist[source] = 0;
}

int dijkstra::minimum() {
    int mini = 10000000;
    int minindex = -1;
    for(int i=0; i<4; i++) {
        if(!visited[i] && dist[i] < mini) {
            mini = dist[i];
            minindex = i;
        }
    }
    return minindex;
}

void dijkstra::shortest_path(int cost[4][4], int source) {
    initialise(cost, source);
    for(int i=0; i<3; i++)
        updation(cost);
}

int main() {
    int cost[4][4];
    cout << "Enter cost matrix:" << endl;
    for(int i=0; i<4; i++)
        for(int j=0; j<4; j++)
            cin >> cost[i][j];
    
    dijkstra d;
    d.shortest_path(cost, 0);
    d.print(0);
    return 0;
}`
    },
    "Max Traffic Flow": {
        problem: "During rush hour, specific highways feeding into the city center get clogged. We need to calculate the theoretical maximum number of vehicles that can travel from the residential suburbs (Source) to the central business district (Sink) without exceeding the capacity of any individual road segment.",
        solution: "This is a classic Max-Flow Min-Cut problem. We use the Ford-Fulkerson algorithm (specifically the Edmonds-Karp implementation using BFS) to find augmenting paths in the residual graph.",
        code: `#include<iostream>
#include<queue>
using namespace std;

#define V 6

bool bfs(int rGraph[V][V], int s, int t, int parent[]) {
    bool visited[V];
    memset(visited, 0, sizeof(visited));
    
    queue<int> q;
    q.push(s);
    visited[s] = true;
    parent[s] = -1;
    
    while(!q.empty()) {
        int u = q.front();
        q.pop();
        
        for(int v=0; v<V; v++) {
            if(!visited[v] && rGraph[u][v] > 0) {
                q.push(v);
                parent[v] = u;
                visited[v] = true;
                if(v == t) return true;
            }
        }
    }
    return false;
}

int fordFulkerson(int graph[V][V], int s, int t) {
    int u, v;
    int rGraph[V][V];
    for(u=0; u<V; u++)
        for(v=0; v<V; v++)
            rGraph[u][v] = graph[u][v];
    
    int parent[V];
    int max_flow = 0;
    
    while(bfs(rGraph, s, t, parent)) {
        int path_flow = INT_MAX;
        for(v=t; v!=s; v=parent[v]) {
            u = parent[v];
            path_flow = min(path_flow, rGraph[u][v]);
        }
        
        for(v=t; v!=s; v=parent[v]) {
            u = parent[v];
            rGraph[u][v] -= path_flow;
            rGraph[v][u] += path_flow;
        }
        max_flow += path_flow;
    }
    return max_flow;
}`
    },
    "Subway Connectivity Check": {
        problem: "The subway system must remain fully connected for efficient transit. If a maintenance event isolates a station or a group of stations from the central hub, passengers could be stranded.",
        solution: "We perform a BFS or DFS starting from the central hub. After traversal, we check if all nodes are visited.",
        code: `#include<iostream>
#include<list>
using namespace std;

class Graph {
    int V;
    list<int> *adj;
    
public:
    Graph(int V);
    void addEdge(int u, int v);
    void BFS(int s);
};

Graph::Graph(int V) {
    this->V = V;
    adj = new list<int>[V];
}

void Graph::addEdge(int u, int v) {
    adj[u].push_back(v);
}

void Graph::BFS(int s) {
    bool *visited = new bool[V];
    for(int i=0; i<V; i++)
        visited[i] = false;
    
    queue<int> q;
    q.push(s);
    visited[s] = true;
    
    while(!q.empty()) {
        int u = q.front();
        q.pop();
        cout << u << " ";
        
        for(auto it = adj[u].begin(); it != adj[u].end(); ++it) {
            if(!visited[*it]) {
                visited[*it] = true;
                q.push(*it);
            }
        }
    }
}`
    },
    "Deadlock Prevention": {
        problem: "In a futuristic grid of autonomous vehicles, circular wait conditions can cause deadlocks.",
        solution: "We use DFS to detect cycles in the resource allocation graph.",
        code: `#include<iostream>
using namespace std;

void dfs(int u, vector<int> adj[], vector<bool>& visited, vector<int>& recStack, bool& hasCycle) {
    visited[u] = true;
    recStack[u] = true;
    
    for(int v : adj[u]) {
        if(!visited[v]) {
            dfs(v, adj, visited, recStack, hasCycle);
        } else if(recStack[v]) {
            hasCycle = true;
        }
    }
    recStack[u] = false;
}

bool detectCycle(int n, vector<int> adj[]) {
    vector<bool> visited(n, false);
    vector<int> recStack(n, false);
    bool hasCycle = false;
    
    for(int i=0; i<n; i++) {
        if(!visited[i]) {
            dfs(i, adj, visited, recStack, hasCycle);
        }
    }
    return hasCycle;
}`
    },
    "Road Network Maintenance": {
        problem: "Determine if two districts are still connected when roads are closed.",
        solution: "Use Disjoint Set Union (DSU) for O(1) connectivity queries.",
        code: `#include<iostream>
using namespace std;

class DSU {
    vector<int> parent, rank;
public:
    DSU(int n) {
        parent.resize(n);
        rank.resize(n, 0);
        for(int i=0; i<n; i++) parent[i] = i;
    }
    
    int find(int x) {
        if(parent[x] != x) parent[x] = find(parent[x]);
        return parent[x];
    }
    
    void unite(int x, int y) {
        int px = find(x), py = find(y);
        if(px == py) return;
        if(rank[px] < rank[py]) parent[px] = py;
        else if(rank[px] > rank[py]) parent[py] = px;
        else { parent[py] = px; rank[px]++; }
    }
    
    bool connected(int x, int y) {
        return find(x) == find(y);
    }
};`
    },
    "Critical Bridge Identification": {
        problem: "Identify critical roads that, if destroyed, would split the city in two.",
        solution: "Use Tarjan's bridge-finding algorithm with DFS.",
        code: `#include<iostream>
using namespace std;

void dfs(int u, int parent, vector<int> adj[], vector<int>& disc, vector<int>& low, vector<pair<int,int>>& bridges, int& timer) {
    disc[u] = low[u] = ++timer;
    
    for(int v : adj[u]) {
        if(v == parent) continue;
        if(disc[v] == -1) {
            dfs(v, u, adj, disc, low, bridges, timer);
            low[u] = min(low[u], low[v]);
            if(low[v] > disc[u])
                bridges.push_back({u, v});
        } else {
            low[u] = min(low[u], disc[v]);
        }
    }
}`
    },
    "Traffic Light Sync": {
        problem: "Prioritize green light duration for busiest intersections.",
        solution: "Use sorting algorithms to rank intersections by throughput.",
        code: `#include<iostream>
#include<vector>
#include<algorithm>
using namespace std;

void mergeSort(vector<int>& arr, int l, int r) {
    if(l < r) {
        int m = l + (r-l)/2;
        mergeSort(arr, l, m);
        mergeSort(arr, m+1, r);
        
        vector<int> left(arr.begin()+l, arr.begin()+m+1);
        vector<int> right(arr.begin()+m+1, arr.begin()+r+1);
        
        int i=0, j=0, k=l;
        while(i<left.size() && j<right.size()) {
            if(left[i] <= right[j]) arr[k++] = left[i++];
            else arr[k++] = right[j++];
        }
        while(i<left.size()) arr[k++] = left[i++];
        while(j<right.size()) arr[k++] = right[j++];
    }
}`
    },
    "A* Search": {
        problem: "Find optimal 3D paths for delivery drones avoiding obstacles.",
        solution: "Use A* algorithm with heuristic guidance towards goal.",
        code: `#include<iostream>
#include<queue>
#include<cmath>
using namespace std;

struct Node {
    int x, y, g, h;
    Node(int _x, int _y, int _g, int _h) : x(_x), y(_y), g(_g), h(_h) {}
    int f() const { return g + h; }
    bool operator<(const Node& other) const { return f() > other.f(); }
};

int heuristic(int x1, int y1, int x2, int y2) {
    return abs(x1-x2) + abs(y1-y2);
}`
    },
    "Congestion Monitoring": {
        problem: "Detect sustained congestion using rolling window analysis.",
        solution: "Use sliding window technique for O(n) average calculation.",
        code: `#include<iostream>
#include<queue>
using namespace std;

class TrafficAnalyzer {
    int K;
    queue<int> window;
    long long sum;
    
public:
    TrafficAnalyzer(int k) : K(k), sum(0) {}
    
    void addReading(int density) {
        window.push(density);
        sum += density;
        
        if((int)window.size() > K) {
            sum -= window.front();
            window.pop();
        }
    }
    
    double getAverage() {
        if(window.empty()) return 0.0;
        return (double)sum / window.size();
    }
};`
    },
    "EV Charging Stations": {
        problem: "Maximize road coverage with limited charging station budget.",
        solution: "Use greedy set cover approximation algorithm.",
        code: `#include<iostream>
#include<set>
#include<vector>
using namespace std;

vector<int> greedySetCover(vector<set<int>>& stations, set<int>& allRoads, int budget) {
    set<int> covered;
    vector<int> chosen;
    
    for(int i=0; i<budget && covered.size() < allRoads.size(); i++) {
        int bestIdx = -1, maxNew = -1;
        
        for(int j=0; j<stations.size(); j++) {
            int newCoverage = 0;
            for(int road : stations[j]) {
                if(covered.find(road) == covered.end())
                    newCoverage++;
            }
            if(newCoverage > maxNew) {
                maxNew = newCoverage;
                bestIdx = j;
            }
        }
        
        if(bestIdx == -1) break;
        chosen.push_back(bestIdx);
        for(int road : stations[bestIdx])
            covered.insert(road);
    }
    return chosen;
}`
    },

    // --- Amogh's Problems ---
    "Grid Connectivity": {
        problem: "Connect all power substations with minimum cable length.",
        solution: "Use Prim's algorithm to find Minimum Spanning Tree.",
        code: `#include<iostream>
using namespace std;

int primMST(vector<vector<pair<int,int>>>& adj, int V) {
    vector<int> key(V, INT_MAX);
    vector<bool> inMST(V, false);
    priority_queue<pair<int,int>, vector<pair<int,int>>, greater<pair<int,int>>> pq;
    
    key[0] = 0;
    pq.push({0, 0});
    int mstWeight = 0;
    
    while(!pq.empty()) {
        int u = pq.top().second;
        pq.pop();
        
        if(inMST[u]) continue;
        inMST[u] = true;
        mstWeight += key[u];
        
        for(auto& edge : adj[u]) {
            int v = edge.first, w = edge.second;
            if(!inMST[v] && w < key[v]) {
                key[v] = w;
                pq.push({key[v], v});
            }
        }
    }
    return mstWeight;
}`
    },
    "Battery Storage Optimization": {
        problem: "Maximize energy storage value under capacity constraints.",
        solution: "Use 0/1 Knapsack dynamic programming.",
        code: `#include<iostream>
using namespace std;

int knapsack(int W, vector<int>& wt, vector<int>& val, int n) {
    vector<int> dp(W+1, 0);
    
    for(int i=0; i<n; i++) {
        for(int w=W; w>=wt[i]; w--) {
            dp[w] = max(dp[w], val[i] + dp[w-wt[i]]);
        }
    }
    return dp[W];
}`
    },
    "Load Balancing": {
        problem: "Assign power requests to substations with minimum load.",
        solution: "Use min-heap priority queue.",
        code: `#include<iostream>
#include<queue>
using namespace std;

priority_queue<int, vector<int>, greater<int>> pq;

void balanceLoad(vector<int>& tasks) {
    for(int task : tasks) {
        int minLoad = pq.top();
        pq.pop();
        minLoad += task;
        pq.push(minLoad);
    }
}`
    },
    "System Boot Sequence": {
        problem: "Determine valid subsystem startup order with dependencies.",
        solution: "Use topological sort with Kahn's algorithm.",
        code: `#include<iostream>
#include<queue>
#include<vector>
using namespace std;

vector<int> topologicalSort(int V, vector<vector<int>>& adj) {
    vector<int> inDegree(V, 0);
    for(int u=0; u<V; u++) {
        for(int v : adj[u])
            inDegree[v]++;
    }
    
    queue<int> q;
    for(int i=0; i<V; i++)
        if(inDegree[i] == 0) q.push(i);
    
    vector<int> result;
    while(!q.empty()) {
        int u = q.front(); q.pop();
        result.push_back(u);
        
        for(int v : adj[u]) {
            if(--inDegree[v] == 0)
                q.push(v);
        }
    }
    return result;
}`
    },
    "Fault Detection": {
        problem: "Find fault location in power line with minimum tests.",
        solution: "Use binary search on linear fault condition.",
        code: `#include<iostream>
using namespace std;

int findBreak(int low, int high) {
    while(low <= high) {
        int mid = low + (high-low)/2;
        if(checkVoltage(mid) == 0)
            high = mid - 1;
        else
            low = mid + 1;
    }
    return low;
}`
    },
    "Peak Usage Query": {
        problem: "Answer max energy usage queries over time ranges.",
        solution: "Use segment tree for O(log n) queries.",
        code: `#include<iostream>
using namespace std;

int query(int node, int start, int end, int l, int r, vector<int>& tree) {
    if(r < start || end < l) return INT_MIN;
    if(l <= start && end <= r) return tree[node];
    
    int mid = (start + end) / 2;
    int p1 = query(2*node, start, mid, l, r, tree);
    int p2 = query(2*node+1, mid+1, end, l, r, tree);
    return max(p1, p2);
}`
    },
    "Smart Meter Compression": {
        problem: "Reduce data transmission from smart meters.",
        solution: "Use Huffman coding compression.",
        code: `#include<iostream>
#include<queue>
using namespace std;

struct Node {
    char ch;
    int freq;
    Node *left, *right;
};

void huffmanCoding(map<char,int>& freq) {
    priority_queue<pair<int,Node*>, vector<pair<int,Node*>>, greater<pair<int,Node*>>> pq;
    
    for(auto& p : freq) {
        Node* newNode = new Node();
        newNode->ch = p.first;
        newNode->freq = p.second;
        pq.push({p.second, newNode});
    }
}`
    },
    "Frequency Allocation": {
        problem: "Assign frequencies to substations avoiding interference.",
        solution: "Use greedy graph coloring.",
        code: `#include<iostream>
using namespace std;

void greedyColoring(vector<vector<int>>& adj, int V) {
    vector<int> result(V, -1);
    result[0] = 0;
    vector<bool> available(V, true);
    
    for(int u=1; u<V; u++) {
        for(int v : adj[u]) {
            if(result[v] != -1)
                available[result[v]] = false;
        }
        
        int color;
        for(color=0; color<V; color++) {
            if(available[color]) break;
        }
        result[u] = color;
        fill(available.begin(), available.end(), true);
    }
}`
    },
    "Real-time Consumption": {
        problem: "Query energy consumption prefix sums dynamically.",
        solution: "Use Fenwick tree for O(log n) updates and queries.",
        code: `#include<iostream>
using namespace std;

class FenwickTree {
    vector<int> bit;
    int n;
public:
    FenwickTree(int n) : n(n), bit(n+1, 0) {}
    
    void update(int i, int delta) {
        for(++i; i<=n; i+=i&-i)
            bit[i] += delta;
    }
    
    int query(int i) {
        int sum = 0;
        for(++i; i>0; i-=i&-i)
            sum += bit[i];
        return sum;
    }
};`
    },
    "Billing & User lookup": {
        problem: "O(1) access to user profiles and energy credits.",
        solution: "Use unordered_map hash table.",
        code: `#include<iostream>
#include<unordered_map>
using namespace std;

struct UserData {
    string name;
    double bill;
};

unordered_map<string, UserData> db;

void addUser(string id, UserData data) {
    db[id] = data;
}

UserData getUser(string id) {
    if(db.find(id) != db.end())
        return db[id];
    return {"", 0.0};
}`
    },

    // --- Subhash's Problems ---
    "Population Density Analysis": {
        problem: "Calculate population in rectangular city regions efficiently.",
        solution: "Use 2D prefix sums for O(1) range queries.",
        code: `#include<iostream>
using namespace std;

int rangeSum(vector<vector<int>>& P, int x1, int y1, int x2, int y2) {
    return P[x2][y2] - P[x1-1][y2] - P[x2][y1-1] + P[x1-1][y1-1];
}`
    },
    "Spatial Indexing": {
        problem: "Find points of interest within radius efficiently.",
        solution: "Use quadtree spatial index.",
        code: `#include<iostream>
using namespace std;

struct Point { int x, y; };

struct QuadTree {
    Point topLeft, botRight;
    QuadTree *nw, *ne, *sw, *se;
    vector<Point> points;
};`
    },
    "City Boundary Logic": {
        problem: "Determine polygon enclosing all city suburbs.",
        solution: "Use convex hull algorithm (monotone chain).",
        code: `#include<iostream>
using namespace std;

vector<Point> convexHull(vector<Point>& pts) {
    sort(pts.begin(), pts.end());
    vector<Point> hull;
    
    for(auto& p : pts) {
        while(hull.size() >= 2 && 
              cross(hull[hull.size()-2], hull.back(), p) <= 0)
            hull.pop_back();
        hull.push_back(p);
    }
    return hull;
}`
    },
    "Housing Allocation": {
        problem: "Allocate budget across house types to maximize utility.",
        solution: "Use unbounded knapsack DP.",
        code: `#include<iostream>
using namespace std;

vector<int> dp(W+1, 0);
for(int i=0; i<=W; i++) {
    for(int j=0; j<n; j++) {
        if(wt[j] <= i)
            dp[i] = max(dp[i], dp[i-wt[j]] + val[j]);
    }
}`
    },
    "Constraint Satisfaction": {
        problem: "Place zones satisfying constraints (N-Queens variant).",
        solution: "Use backtracking algorithm.",
        code: `#include<iostream>
using namespace std;

bool solveNQUtil(vector<vector<int>>& board, int col) {
    if(col >= N) return true;
    
    for(int i=0; i<N; i++) {
        if(isSafe(board, i, col)) {
            board[i][col] = 1;
            if(solveNQUtil(board, col+1)) return true;
            board[i][col] = 0;
        }
    }
    return false;
}`
    },
    "Skyline Problem": {
        problem: "Compute city skyline contour from building coordinates.",
        solution: "Use sweep line with max-heap.",
        code: `#include<iostream>
#include<multiset>
using namespace std;

vector<pair<int,int>> getSkyline(vector<vector<int>>& buildings) {
    multiset<int> heights = {0};
    vector<pair<int,int>> result;
    int prevMax = 0;
    
    for(auto& b : buildings) {
        heights.insert(b[2]);
        int curMax = *heights.rbegin();
        if(curMax != prevMax) {
            result.push_back({b[0], curMax});
            prevMax = curMax;
        }
    }
    return result;
}`
    },
    "Service Center Locations": {
        problem: "Group neighborhoods into K clusters for service centers.",
        solution: "Use K-means clustering algorithm.",
        code: `#include<iostream>
using namespace std;

void kMeans(vector<Point>& points, int K) {
    vector<Point> centroids(K);
    bool changed = true;
    
    while(changed) {
        for(auto& p : points)
            p.cluster = nearestCentroid(p, centroids);
        
        changed = false;
        for(int k=0; k<K; k++) {
            Point newCentroid = average(pointsInCluster(k));
            if(newCentroid != centroids[k]) changed = true;
            centroids[k] = newCentroid;
        }
    }
}`
    },
    "Job Assignment": {
        problem: "Assign construction crews to sites based on skills.",
        solution: "Use max bipartite matching.",
        code: `#include<iostream>
using namespace std;

bool bpm(int u, bool visited[], int matchR[], vector<vector<int>>& bpGraph) {
    for(int v=0; v<N; v++) {
        if(bpGraph[u][v] && !visited[v]) {
            visited[v] = true;
            if(matchR[v] < 0 || bpm(matchR[v], visited, matchR, bpGraph)) {
                matchR[v] = u;
                return true;
            }
        }
    }
    return false;
}`
    },
    "Overlap Detection": {
        problem: "Detect intersecting utility lines in blueprints.",
        solution: "Use sweep line algorithm.",
        code: `#include<iostream>
using namespace std;

bool hasIntersection(vector<Segment>& segments) {
    vector<Event> events;
    for(auto& seg : segments) {
        events.push_back({seg.x1, START, seg});
        events.push_back({seg.x2, END, seg});
    }
    sort(events.begin(), events.end());
    
    set<Segment> active;
    for(auto& e : events) {
        if(e.type == START) {
            active.insert(e.segment);
        } else {
            active.erase(e.segment);
        }
    }
}`
    },
    "Flood Risk Assessment": {
        problem: "Identify flood-prone areas using terrain flow simulation.",
        solution: "Use DFS/BFS matrix traversal.",
        code: `#include<iostream>
using namespace std;

void dfs(int r, int c, vector<vector<int>>& h, vector<vector<bool>>& visited) {
    visited[r][c] = true;
    int dr[] = {0,0,1,-1};
    int dc[] = {1,-1,0,0};
    
    for(int i=0; i<4; i++) {
        int nr = r+dr[i], nc = c+dc[i];
        if(isValid(nr,nc) && !visited[nr][nc] && h[nr][nc] <= h[r][c])
            dfs(nr, nc, h, visited);
    }
}`
    },

    // --- Abhinav's Problems ---
    "Emergency Dispatch": {
        problem: "Dispatch nearest ambulance prioritizing by severity.",
        solution: "Use priority queue with custom comparator.",
        code: `#include<iostream>
#include<queue>
using namespace std;

struct Event {
    int severity, distance, id;
    bool operator<(const Event& o) const {
        if(severity != o.severity) return severity < o.severity;
        return distance > o.distance;
    }
};

priority_queue<Event> pq;`
    },
    "Garbage Truck Routing": {
        problem: "Find minimum-distance route visiting all dumpsters.",
        solution: "Use TSP with DP and bitmask.",
        code: `#include<iostream>
using namespace std;

int tsp(int mask, int pos, int n, vector<vector<int>>& dist, vector<vector<int>>& dp) {
    if(mask == (1<<n)-1) return dist[pos][0];
    if(dp[mask][pos] != -1) return dp[mask][pos];
    
    int ans = INT_MAX;
    for(int city=0; city<n; city++) {
        if((mask & (1<<city)) == 0) {
            int newAns = dist[pos][city] + tsp(mask|(1<<city), city, n, dist, dp);
            ans = min(ans, newAns);
        }
    }
    return dp[mask][pos] = ans;
}`
    },
    "Fire Station Coverage": {
        problem: "Find minimum time to reach any building from any station.",
        solution: "Use multi-source BFS.",
        code: `#include<iostream>
#include<queue>
using namespace std;

void multiSourceBFS(vector<int>& stations, vector<vector<int>>& adj, vector<int>& dist) {
    queue<int> q;
    for(int s : stations) {
        q.push(s);
        dist[s] = 0;
    }
    
    while(!q.empty()) {
        int u = q.front(); q.pop();
        for(int v : adj[u]) {
            if(dist[v] == INT_MAX) {
                dist[v] = dist[u] + 1;
                q.push(v);
            }
        }
    }
}`
    },
    "Crime Hotspot Prediction": {
        problem: "Classify location as safe/risky using nearest incidents.",
        solution: "Use K-nearest neighbors classification.",
        code: `#include<iostream>
using namespace std;

bool predictRisk(Point query, vector<IncidentData>& data, int K) {
    vector<pair<double,bool>> distances;
    for(auto& p : data) {
        distances.push_back({dist(query, p.coords), p.isRisk});
    }
    sort(distances.begin(), distances.end());
    
    int riskCount = 0;
    for(int i=0; i<K; i++) {
        if(distances[i].second) riskCount++;
    }
    return riskCount > K/2;
}`
    },
    "Emergency Contact Lookup": {
        problem: "Autocomplete names/addresses during 911 calls.",
        solution: "Use trie data structure.",
        code: `#include<iostream>
using namespace std;

struct TrieNode {
    TrieNode *children[26];
    bool isEndOfWord;
};

void insert(TrieNode *root, string key) {
    TrieNode *pCrawl = root;
    for(int i=0; i<key.length(); i++) {
        int index = key[i] - 'a';
        if(!pCrawl->children[index])
            pCrawl->children[index] = new TrieNode();
        pCrawl = pCrawl->children[index];
    }
    pCrawl->isEndOfWord = true;
}`
    },
    "Hospital Bed Management": {
        problem: "Maximize scheduled surgeries given time constraints.",
        solution: "Use greedy interval scheduling.",
        code: `#include<iostream>
#include<algorithm>
using namespace std;

int maxSurgeries(vector<pair<int,int>>& intervals) {
    sort(intervals.begin(), intervals.end(), 
         [](auto& a, auto& b) { return a.second < b.second; });
    
    int count = 1, lastEnd = intervals[0].second;
    for(int i=1; i<intervals.size(); i++) {
        if(intervals[i].first >= lastEnd) {
            count++;
            lastEnd = intervals[i].second;
        }
    }
    return count;
}`
    },
    "Dispatch Logs": {
        problem: "Maintain fixed-size buffer of recent emergency calls.",
        solution: "Use circular queue.",
        code: `#include<iostream>
using namespace std;

class CircularQueue {
    vector<int> arr;
    int front, rear, size;
    
public:
    void enqueue(int val) {
        if((rear+1)%size == front) return;
        if(front == -1) front = 0;
        rear = (rear+1)%size;
        arr[rear] = val;
    }
    
    int dequeue() {
        if(front == -1) return -1;
        int val = arr[front];
        if(front == rear) front = rear = -1;
        else front = (front+1)%size;
        return val;
    }
};`
    },
    "Disaster Evacuation": {
        problem: "Pre-calculate all shortest evacuation paths.",
        solution: "Use Floyd-Warshall all-pairs shortest path.",
        code: `#include<iostream>
using namespace std;

void floydWarshall(vector<vector<int>>& dist, int V) {
    for(int k=0; k<V; k++) {
        for(int i=0; i<V; i++) {
            for(int j=0; j<V; j++) {
                if(dist[i][k] + dist[k][j] < dist[i][j])
                    dist[i][j] = dist[i][k] + dist[k][j];
            }
        }
    }
}`
    },
    "Duplicate Incident Filtering": {
        problem: "Ignore duplicate 911 reports in real-time.",
        solution: "Use hash set for O(1) duplicate detection.",
        code: `#include<iostream>
#include<unordered_set>
using namespace std;

unordered_set<string> activeIncidents;

bool processIncident(int x, int y, string type) {
    string id = to_string(x) + "_" + to_string(y) + "_" + type;
    
    if(activeIncidents.find(id) == activeIncidents.end()) {
        activeIncidents.insert(id);
        dispatch();
        return true;
    }
    return false;
}`
    },
    "Medical Data Access": {
        problem: "Cache recent patient records with limited memory.",
        solution: "Use LRU cache with hash map + doubly linked list.",
        code: `#include<iostream>
#include<unordered_map>
#include<list>
using namespace std;

class LRUCache {
    int capacity;
    unordered_map<int, list<pair<int,int>>::iterator> map;
    list<pair<int,int>> cache;
    
public:
    LRUCache(int cap) : capacity(cap) {}
    
    int get(int key) {
        if(map.find(key) == map.end()) return -1;
        cache.splice(cache.begin(), cache, map[key]);
        return map[key]->second;
    }
    
    void put(int key, int val) {
        if(map.find(key) != map.end()) {
            cache.splice(cache.begin(), cache, map[key]);
            map[key]->second = val;
        } else {
            if(cache.size() == capacity) {
                int delKey = cache.back().first;
                cache.pop_back();
                map.erase(delKey);
            }
            cache.push_front({key, val});
            map[key] = cache.begin();
        }
    }
};`
    }
};
