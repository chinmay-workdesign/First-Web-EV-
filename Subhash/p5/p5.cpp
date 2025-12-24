#include <iostream>
#include <vector>
using namespace std;

/*
MODEL 5: Spatial Knowledge Grid (HSKG)
Techniques:
1) Quadtree
2) Hierarchical Tree Traversal
*/

struct Point {
    int x, y;
};

struct QuadTree {
    int x1, y1, x2, y2;        // boundary
    vector<Point> points;     // stored assets
    QuadTree *nw, *ne, *sw, *se;

    QuadTree(int _x1, int _y1, int _x2, int _y2) {
        x1 = _x1; y1 = _y1; x2 = _x2; y2 = _y2;
        nw = ne = sw = se = nullptr;
    }
};

/* ---------- CHECK IF POINT IS INSIDE ---------- */
bool inside(QuadTree* node, Point p) {
    return p.x >= node->x1 && p.x <= node->x2 &&
           p.y >= node->y1 && p.y <= node->y2;
}

/* ---------- SUBDIVIDE ---------- */
void subdivide(QuadTree* node) {
    int midX = (node->x1 + node->x2) / 2;
    int midY = (node->y1 + node->y2) / 2;

    node->nw = new QuadTree(node->x1, node->y1, midX, midY);
    node->ne = new QuadTree(midX+1, node->y1, node->x2, midY);
    node->sw = new QuadTree(node->x1, midY+1, midX, node->y2);
    node->se = new QuadTree(midX+1, midY+1, node->x2, node->y2);
}

/* ---------- INSERT POINT ---------- */
void insert(QuadTree* node, Point p) {
    if (!inside(node, p)) return;

    if (node->points.size() < 1 && node->nw == nullptr) {
        node->points.push_back(p);
        return;
    }

    if (node->nw == nullptr)
        subdivide(node);

    insert(node->nw, p);
    insert(node->ne, p);
    insert(node->sw, p);
    insert(node->se, p);
}

/* ---------- RANGE QUERY ---------- */
void rangeQuery(QuadTree* node, int qx1, int qy1, int qx2, int qy2) {
    if (node == nullptr) return;

    // no overlap
    if (node->x2 < qx1 || node->x1 > qx2 ||
        node->y2 < qy1 || node->y1 > qy2)
        return;

    for (auto &p : node->points) {
        if (p.x >= qx1 && p.x <= qx2 &&
            p.y >= qy1 && p.y <= qy2) {
            cout << "Asset at (" << p.x << "," << p.y << ")\n";
        }
    }

    rangeQuery(node->nw, qx1, qy1, qx2, qy2);
    rangeQuery(node->ne, qx1, qy1, qx2, qy2);
    rangeQuery(node->sw, qx1, qy1, qx2, qy2);
    rangeQuery(node->se, qx1, qy1, qx2, qy2);
}

/* ---------- MAIN ---------- */
int main() {

    // City boundary (0,0) to (100,100)
    QuadTree* city = new QuadTree(0, 0, 100, 100);

    // Insert city assets (parks, schools, hospitals)
    vector<Point> assets = {
        {10,20}, {15,80}, {50,50},
        {70,20}, {90,90}, {30,40}
    };

    for (auto &p : assets)
        insert(city, p);

    cout << "Assets in region (10,10) to (60,60):\n";
    rangeQuery(city, 10, 10, 60, 60);

    return 0;
}
