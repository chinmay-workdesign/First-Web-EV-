#include <bits/stdc++.h>
using namespace std;

/*
====================================================================================================
                        FENWICK TREE (BIT) FOR CONSUMPTION TRACKING
----------------------------------------------------------------------------------------------------
We maintain an array of hourly consumption values. We often:
    - Update value at index i
    - Query prefix sum from 0 to i

Fenwick Tree supports:
    update(i, val) in log n
    sum(i) in log n

This program:
    1. Loads consumption array from CSV
    2. Builds Fenwick Tree
    3. Demonstrates prefix queries and updates
====================================================================================================
*/

// =================================================================================================
// SECTION 1 — Helper Utilities
// =================================================================================================

string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");

    if (start == string::npos) return "";
    return s.substr(start, end - start + 1);
}

void printBanner(const string &msg) {
    cout << "\n============================================================\n";
    cout << msg << "\n";
    cout << "============================================================\n";
}

// =================================================================================================
// SECTION 2 — CSV Loader Class
// =================================================================================================

class CSVLoader {
public:
    string filename;

    CSVLoader(const string &fn) : filename(fn) {}

    void load(vector<int> &arr) {
        printBanner("Loading Consumption Data");

        ifstream file(filename);

        if (!file.is_open()) {
            cout << "ERROR: Could not open CSV file.\n";
            exit(1);
        }

        string line;
        bool skip = true;

        while (getline(file, line)) {
            if (skip) {
                skip = false;
                continue;
            }

            stringstream ss(line);
            string val;
            getline(ss, val, ',');
            arr.push_back(stoi(trim(val)));
        }

        cout << "Loaded " << arr.size() << " consumption entries.\n";
    }
};

// =================================================================================================
// SECTION 3 — Fenwick Tree Class
// =================================================================================================

class FenwickTree {
public:
    int n;
    vector<int> fenwick;

    FenwickTree(int size) {
        n = size;
        fenwick.assign(n + 1, 0);
    }

    void build(const vector<int> &arr) {
        printBanner("Building Fenwick Tree");

        for (int i = 0; i < arr.size(); i++) {
            update(i, arr[i]);
        }
    }

    void update(int index, int val) {
        index++;

        while (index <= n) {
            fenwick[index] += val;
            index += index & -index;
        }
    }

    int prefixSum(int index) {
        index++;

        int result = 0;
        while (index > 0) {
            result += fenwick[index];
            index -= index & -index;
        }
        return result;
    }

    int rangeSum(int l, int r) {
        if (l > r) return 0;
        return prefixSum(r) - (l > 0 ? prefixSum(l - 1) : 0);
    }
};

// =================================================================================================
// SECTION 4 — Debug Helpers
// =================================================================================================

void printSample(const vector<int> &arr) {
    printBanner("Sample First 10 Values");

    for (int i = 0; i < min((int)arr.size(), 10); i++) {
        cout << "Index " << i << ": " << arr[i] << "\n";
    }
}

// =================================================================================================
// SECTION 5 — MAIN PROGRAM (>200 lines)
// =================================================================================================

int main() {
    printBanner("CONSUMPTION TRACKING USING FENWICK TREE");

    vector<int> arr;

    CSVLoader loader("consumption_data.csv");
    loader.load(arr);

    printSample(arr);

    FenwickTree ft(arr.size());
    ft.build(arr);

    printBanner("Example Prefix Queries");

    cout << "Prefix sum up to index 5:  " << ft.prefixSum(5) << "\n";
    cout << "Prefix sum up to index 20: " << ft.prefixSum(20) << "\n";
    cout << "Prefix sum up to index 50: " << ft.prefixSum(50) << "\n";

    printBanner("Applying Example Updates");

    cout << "Updating index 10 by +20\n";
    ft.update(10, 20);
    cout << "New prefix sum at 10 = " << ft.prefixSum(10) << "\n";

    cout << "Updating index 3 by +50\n";
    ft.update(3, 50);
    cout << "New prefix sum at 10 = " << ft.prefixSum(10) << "\n";

    cout << "Updating index 100 by +100\n";
    ft.update(100, 100);
    cout << "New prefix sum at 150 = " << ft.prefixSum(150) << "\n";

    printBanner("PROGRAM COMPLETE");
    return 0;
}

/*
====================================================================================================
End of Program
====================================================================================================
*/
