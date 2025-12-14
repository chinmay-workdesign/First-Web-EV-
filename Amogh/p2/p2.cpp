#include <bits/stdc++.h>
using namespace std;

/*
====================================================================================
                            0/1 KNAPSACK FOR ENERGY STORAGE
------------------------------------------------------------------------------------
====================================================================================
*/

// -----------------------------------------------------------------------------
// SECTION 1 — Helper String Functions
// -----------------------------------------------------------------------------

// Trims whitespace from both ends (useful for CSV safety)
string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    size_t end   = s.find_last_not_of(" \t\n\r");
    if (start == string::npos) return "";
    return s.substr(start, end - start + 1);
}

// Prints big section banners for clarity
void printBanner(const string &title) {
    cout << "\n============================================================\n";
    cout << title << "\n";
    cout << "============================================================\n";
}

// -----------------------------------------------------------------------------
// SECTION 2 — Data Loader Class (Loads CSV into vectors)
// -----------------------------------------------------------------------------

class CSVLoader {
public:
    string filename;

    CSVLoader(const string &file) : filename(file) {}

    // Loads weight/value pairs from CSV
    void load(vector<int> &weights, vector<int> &values) {
        printBanner("Loading CSV File: " + filename);

        ifstream file(filename);
        if (!file.is_open()) {
            cout << "ERROR: Cannot open CSV file!" << endl;
            exit(1);
        }

        string line;
        bool skipHeader = true;

        while (getline(file, line)) {
            if (skipHeader) { skipHeader = false; continue; }

            stringstream ss(line);
            string wStr, vStr;

            getline(ss, wStr, ',');
            getline(ss, vStr, ',');

            int w = stoi(trim(wStr));
            int v = stoi(trim(vStr));

            weights.push_back(w);
            values.push_back(v);
        }

        cout << "CSV loaded successfully: " 
             << weights.size() << " items found.\n";
    }
};

// -----------------------------------------------------------------------------
// SECTION 3 — Knapsack Solver Class (Core DP Logic)
// -----------------------------------------------------------------------------

class KnapsackSolver {
public:
    vector<int> weights;
    vector<int> values;
    int capacity;

    vector<int> dp;  // DP array (1D)

    KnapsackSolver(int cap) {
        capacity = cap;
        dp.resize(capacity + 1, 0);
    }

    // Assign loaded inputs
    void setItems(const vector<int> &w, const vector<int> &v) {
        weights = w;
        values = v;
    }

    // Main DP computation
    void compute() {
        printBanner("Computing 0/1 Knapsack Using Dynamic Programming");

        int n = weights.size();

        for (int i = 0; i < n; i++) {
            for (int w = capacity; w >= weights[i]; w--) {
                dp[w] = max(dp[w], values[i] + dp[w - weights[i]]);
            }
        }
    }

    // Output result
    void printResult() {
        printBanner("RESULT — MAXIMUM ENERGY VALUE STORED");
        cout << "Maximum energy value = " << dp[capacity] << "\n";
    }
};

// -----------------------------------------------------------------------------
// SECTION 4 — Utility (Print sample items)
// -----------------------------------------------------------------------------

void printSampleItems(const vector<int> &weights, const vector<int> &values) {
    printBanner("Sample of Loaded Items (First 10)");

    for (int i = 0; i < min((int)weights.size(), 10); i++) {
        cout << "Item " << i+1 
             << " | Weight = " << weights[i]
             << " | Value = " << values[i] << "\n";
    }
}

// -----------------------------------------------------------------------------
// SECTION 5 — MAIN FUNCTION 
// -----------------------------------------------------------------------------

int main() {

    printBanner("0/1 KNAPSACK — ENERGY STORAGE OPTIMIZATION");

    // Battery capacity (you may modify)
    int W = 1000;

    vector<int> weights;
    vector<int> values;

    // Load CSV
    CSVLoader loader("knapsack_1000_inputs.csv");
    loader.load(weights, values);

    // Print sample items
    printSampleItems(weights, values);

    // Initialize solver
    KnapsackSolver solver(W);
    solver.setItems(weights, values);

    // Run DP
    solver.compute();

    // Print Output
    solver.printResult();

    printBanner("PROGRAM COMPLETED SUCCESSFULLY");
    return 0;
}

/*
====================================================================================
END OF PROGRAM  
====================================================================================
*/
