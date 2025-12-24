#include <iostream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <algorithm>
using namespace std;

/*
MODEL 6: IHDOF – Housing Distribution Optimizer
Goal:
Maximize housing utility under limited land/budget
*/

struct Housing {
    string type;
    int cost;
    int value;
};

int main() {

    // ---------------- BASIC INPUT ----------------
    int budget = 12;   // total land / budget capacity

    vector<Housing> houses = {
        {"Affordable", 3, 30},
        {"Premium",    6, 70},
        {"MixedUse",   4, 50}
    };

    int n = houses.size();

    // ---------------- HASH MAP (zone constraints / metadata) ----------------
    unordered_map<string, int> zoneLimit;
    zoneLimit["Affordable"] = 3;
    zoneLimit["Premium"] = 2;
    zoneLimit["MixedUse"] = 2;

    // ---------------- PREFIX SUM (capacity tracking) ----------------
    vector<int> prefixCost(n + 1, 0);
    for (int i = 1; i <= n; i++) {
        prefixCost[i] = prefixCost[i - 1] + houses[i - 1].cost;
    }

    // ---------------- SORTING (Greedy priority by value density) ----------------
    sort(houses.begin(), houses.end(), [](Housing &a, Housing &b) {
        return (double)a.value / a.cost > (double)b.value / b.cost;
    });

    // ---------------- PRIORITY QUEUE (Max-Heap) ----------------
    priority_queue<pair<int,string>> pq;
    for (auto &h : houses) {
        pq.push({h.value, h.type});
    }

    // ---------------- DYNAMIC PROGRAMMING ----------------
    vector<int> dp(budget + 1, 0);

    for (int b = 1; b <= budget; b++) {
        for (auto &h : houses) {
            if (h.cost <= b) {
                dp[b] = max(dp[b], dp[b - h.cost] + h.value);
            }
        }
    }

    // ---------------- BINARY SEARCH (minimum budget for utility threshold) ----------------
    int targetUtility = 120;
    int minBudget = -1;
    int low = 0, high = budget;

    while (low <= high) {
        int mid = (low + high) / 2;
        if (dp[mid] >= targetUtility) {
            minBudget = mid;
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    // ---------------- OUTPUT ----------------
    cout << "=== IHDOF : Housing Distribution Optimizer ===\n\n";

    cout << "Total Budget Capacity: " << budget << endl;
    cout << "Maximum Housing Utility: " << dp[budget] << endl;

    cout << "\nDP Table (Budget -> Utility):\n";
    for (int i = 0; i <= budget; i++) {
        cout << "Budget " << i << " : " << dp[i] << endl;
    }

    cout << "\nHighest Priority Housing Types (Heap):\n";
    while (!pq.empty()) {
        cout << pq.top().second << " (Utility " << pq.top().first << ")\n";
        pq.pop();
    }

    if (minBudget != -1)
        cout << "\nMinimum Budget needed for utility " << targetUtility << " is: " << minBudget << endl;
    else
        cout << "\nTarget utility not achievable within budget.\n";

    return 0;
}
