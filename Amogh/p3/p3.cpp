#include <bits/stdc++.h>
using namespace std;

/*
===================================================================================================
                    MIN-HEAP BASED LOAD BALANCING SYSTEM
---------------------------------------------------------------------------------------------------
===================================================================================================
*/

// =================================================================================================
// SECTION 1 — Utility: String Trimming
// =================================================================================================

string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    size_t end   = s.find_last_not_of(" \t\n\r");
    if (start == string::npos) return "";
    return s.substr(start, end - start + 1);
}

// =================================================================================================
// SECTION 2 — Banner Printer
// =================================================================================================

void printBanner(const string &title) {
    cout << "\n============================================================\n";
    cout << title << "\n";
    cout << "============================================================\n";
}

// =================================================================================================
// SECTION 3 — CSV Loader Class
// =================================================================================================

class CSVLoader {
public:
    string filename;

    CSVLoader(const string &file) : filename(file) {}

    void loadTasks(vector<int> &tasks) {
        printBanner("Loading Tasks from CSV");

        ifstream file(filename);
        if (!file.is_open()) {
            cout << "ERROR: Cannot open CSV file!" << endl;
            exit(1);
        }

        string line;
        bool skip = true;

        while (getline(file, line)) {
            if (skip) { skip = false; continue; }

            stringstream ss(line);
            string loadStr;

            getline(ss, loadStr, ',');

            int loadValue = stoi(trim(loadStr));
            tasks.push_back(loadValue);
        }

        cout << "CSV Loaded Successfully. Task Count = " << tasks.size() << "\n";
    }
};

// =================================================================================================
// SECTION 4 — Load Balancer Class (Greedy + Min-Heap)
// =================================================================================================

class LoadBalancer {
public:
    int numServers;
    vector<int> serverLoad;

    // Min-Heap stores {load, server_id}
    priority_queue<pair<int,int>, vector<pair<int,int>>, greater<pair<int,int>>> minHeap;

    LoadBalancer(int servers) {
        numServers = servers;
        serverLoad.resize(numServers, 0);

        for (int i = 0; i < numServers; i++) {
            minHeap.push({0, i});
        }
    }

    void assignTask(int load) {
        auto top = minHeap.top();
        minHeap.pop();

        int currentLoad = top.first;
        int serverId = top.second;

        currentLoad += load;
        serverLoad[serverId] = currentLoad;

        minHeap.push({currentLoad, serverId});
    }

    void assignAllTasks(const vector<int> &tasks) {
        printBanner("Assigning Tasks to Servers");

        for (int task : tasks) {
            assignTask(task);
        }
    }

    void printServerLoads() {
        printBanner("Final Server Loads");

        for (int i = 0; i < numServers; i++) {
            cout << "Server " << i + 1 
                 << " Load = " << serverLoad[i] << "\n";
        }
    }

    int getMinLoadServer() {
        return minHeap.top().second;
    }
};

// =================================================================================================
// SECTION 5 — Sample Printer for First 10 Tasks
// =================================================================================================

void printTaskSample(const vector<int> &tasks) {
    printBanner("Sample of First 10 Tasks");

    for (int i = 0; i < min((int)tasks.size(), 10); i++) {
        cout << "Task " << i + 1 << " → Load = " << tasks[i] << "\n";
    }
}

// =================================================================================================
// SECTION 6 — MAIN 
// =================================================================================================

int main() {
    printBanner("LOAD BALANCING USING MIN HEAP");

    vector<int> tasks;

    CSVLoader loader("task_loads_1000.csv");
    loader.loadTasks(tasks);

    printTaskSample(tasks);

    int numberOfServers = 10;  // example value
    LoadBalancer balancer(numberOfServers);

    balancer.assignAllTasks(tasks);

    balancer.printServerLoads();

    printBanner("PROGRAM COMPLETED SUCCESSFULLY");
    return 0;
}

/*
===================================================================================================
END OF FILE  
===================================================================================================
*/
