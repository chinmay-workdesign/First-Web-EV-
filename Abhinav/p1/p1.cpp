// ambulance_dispatch.cpp
// Compile: g++ -std=c++17 -O2 -o ambulance_dispatch ambulance_dispatch.cpp
// Usage: ./ambulance_dispatch incidents.csv
#include <bits/stdc++.h>
using namespace std;

struct Incident {
    string id;
    int severity;     // higher means more critical
    double x, y;
    long timestamp;
};

struct Ambulance {
    string id;
    double x, y;
    bool available;
};

struct Assignment {
    string incident_id;
    string ambulance_id;
    double distance;
    long assigned_time;
};

struct IncidentComparator {
    // priority: higher severity first. If equal, older timestamp first.
    bool operator()(Incident const &a, Incident const &b) const {
        if (a.severity != b.severity) return a.severity < b.severity; // max-heap
        return a.timestamp > b.timestamp;
    }
};

double euclidDist(const Incident &ins, const Ambulance &amb) {
    double dx = ins.x - amb.x;
    double dy = ins.y - amb.y;
    return sqrt(dx*dx + dy*dy);
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " incidents.csv\n";
        return 1;
    }

    string incidents_file = argv[1];

    // 1) Read incidents CSV
    vector<Incident> incidents;
    {
        ifstream fin(incidents_file);
        if (!fin) {
            cerr << "Failed to open " << incidents_file << "\n";
            return 1;
        }
        string header;
        getline(fin, header); // skip header
        string line;
        while (getline(fin, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string id, sev_s, x_s, y_s, ts_s;
            // CSV: incident_id,severity,x,y,timestamp
            getline(ss, id, ',');
            getline(ss, sev_s, ',');
            getline(ss, x_s, ',');
            getline(ss, y_s, ',');
            getline(ss, ts_s, ',');
            Incident ins;
            ins.id = id;
            ins.severity = stoi(sev_s);
            ins.x = stod(x_s);
            ins.y = stod(y_s);
            ins.timestamp = stol(ts_s);
            incidents.push_back(ins);
        }
        cout << "Loaded " << incidents.size() << " incidents from " << incidents_file << "\n";
    }

    // 2) Create some ambulances (for simulation). In production, read from CSV / DB.
    vector<Ambulance> ambulances;
    {
        // Example: 200 ambulances uniformly distributed
        int NUM_AMB = 200;
        ambulances.reserve(NUM_AMB);
        std::mt19937 rng(12345);
        std::uniform_real_distribution<double> distXY(0.0, 100000.0);
        for (int i = 0; i < NUM_AMB; ++i) {
            Ambulance a;
            a.id = "AMB" + to_string(i+1);
            a.x = distXY(rng);
            a.y = distXY(rng);
            a.available = true;
            ambulances.push_back(a);
        }
        cout << "Generated " << ambulances.size() << " ambulances\n";
    }

    // 3) Build priority queue of incidents
    priority_queue<Incident, vector<Incident>, IncidentComparator> pq;
    for (auto &ins : incidents) pq.push(ins);

    // 4) Dispatch loop: pop highest-priority incident and find nearest available ambulance
    vector<Assignment> assignments;
    assignments.reserve(min((size_t)ambulances.size(), pq.size()));

    while (!pq.empty()) {
        Incident ins = pq.top(); pq.pop();
        // Find nearest available ambulance (linear scan)
        double bestDist = numeric_limits<double>::infinity();
        int bestIdx = -1;
        for (size_t i = 0; i < ambulances.size(); ++i) {
            if (!ambulances[i].available) continue;
            double d = euclidDist(ins, ambulances[i]);
            if (d < bestDist) {
                bestDist = d;
                bestIdx = (int)i;
            }
        }
        if (bestIdx == -1) {
            // No ambulance available at the moment
            Assignment a;
            a.incident_id = ins.id;
            a.ambulance_id = "NONE";
            a.distance = -1.0;
            a.assigned_time = ins.timestamp;
            assignments.push_back(a);
            continue;
        }
        // Assign ambulance
        ambulances[bestIdx].available = false; // mark busy (for this demo we never free)
        Assignment a;
        a.incident_id = ins.id;
        a.ambulance_id = ambulances[bestIdx].id;
        a.distance = bestDist;
        a.assigned_time = ins.timestamp;
        assignments.push_back(a);
    }

    // 5) Output assignments (print first 50 and save to CSV)
    cout << "Total assignments: " << assignments.size() << "\n";
    string out_csv = "assignments.csv";
    ofstream fout(out_csv);
    fout << "incident_id,ambulance_id,distance,assigned_time\n";
    int cnt = 0;
    for (auto &as : assignments) {
        fout << as.incident_id << "," << as.ambulance_id << "," << as.distance << "," << as.assigned_time << "\n";
        if (cnt < 50) {
            cout << as.incident_id << " -> " << as.ambulance_id << " dist=" << as.distance << " at " << as.assigned_time << "\n";
        }
        cnt++;
    }
    fout.close();
    cout << "Assignments written to " << out_csv << "\n";

    return 0;
}
