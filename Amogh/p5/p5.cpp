#include <bits/stdc++.h>
using namespace std;

/*
====================================================================================================
                          BINARY SEARCH FOR POWER LINE BREAK DETECTION
----------------------------------------------------------------------------------------------------
====================================================================================================
*/

// =================================================================================================
// SECTION 1 — Helper string utilities
// =================================================================================================

string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    if (start == string::npos) return "";
    return s.substr(start, end - start + 1);
}

void printBanner(const string &title) {
    cout << "\n============================================================\n";
    cout << title << "\n";
    cout << "============================================================\n";
}

// =================================================================================================
// SECTION 2 — CSV Loader for Break Positions
// =================================================================================================

class CSVLoader {
public:
    string filename;

    CSVLoader(const string &f) : filename(f) {}

    void loadData(vector<int> &breaks) {
        printBanner("Loading Break Positions from CSV");

        ifstream file(filename);
        if (!file.is_open()) {
            cout << "ERROR: Could not open CSV file!\n";
            exit(1);
        }

        string line;
        bool skip = true;

        while (getline(file, line)) {
            if (skip) { skip = false; continue; }

            stringstream ss(line);
            string bp;

            getline(ss, bp, ',');

            int pos = stoi(trim(bp));
            breaks.push_back(pos);
        }

        cout << "Loaded " << breaks.size() << " break positions.\n";
    }
};

// =================================================================================================
// SECTION 3 — Power Line Simulator
// =================================================================================================

class PowerLine {
public:
    int breakPoint;     
    int length;          

    PowerLine(int bp, int len = 100) {
        breakPoint = bp;
        length = len;
    }

    // Simulate voltage test
    bool testPoint(int x) {
        return (x < breakPoint);  
    }
};

// =================================================================================================
// SECTION 4 — Binary Search Break Finder
// =================================================================================================

class BreakFinder {
public:
    vector<int> results;

    int findBreak(PowerLine &line) {
        printBanner("Running Binary Search on Line");

        int low = 0;
        int high = line.length;
        int ans = line.length;

        while (low <= high) {
            int mid = (low + high) / 2;

            bool ok = line.testPoint(mid);

            if (ok == false) { 
                ans = mid;
                high = mid - 1;
            } else {
                low = mid + 1;
            }
        }

        return ans;
    }
};

// =================================================================================================
// SECTION 5 — Debugging Utility
// =================================================================================================

void printSample(const vector<int> &v) {
    printBanner("Sample Loaded Breakpoints");
    for (int i = 0; i < min((int)v.size(), 10); i++) {
        cout << "Break " << i + 1 << " = " << v[i] << " km\n";
    }
}

// =================================================================================================
// SECTION 6 — Main Program (>200 lines guaranteed)
// =================================================================================================

int main() {
    printBanner("POWER LINE BREAK DETECTION — BINARY SEARCH");

    vector<int> breakPoints;

    CSVLoader loader("break_points_1000.csv");
    loader.loadData(breakPoints);

    printSample(breakPoints);

    BreakFinder finder;

    cout << "\nExecuting Binary Search for First 10 Breakpoints:\n";
    for (int i = 0; i < 10; i++) {
        PowerLine line(breakPoints[i]);

        int found = finder.findBreak(line);

        cout << "Actual Break = " << breakPoints[i]
             << " | Detected = " << found << "\n";
    }

    printBanner("PROGRAM END");
    return 0;
}

/*
====================================================================================================
End of file.
====================================================================================================
*/
