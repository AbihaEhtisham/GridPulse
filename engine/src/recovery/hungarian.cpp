#include "recovery/hungarian.hpp"
#include <algorithm>
#include <cmath>

namespace gridpulse {

std::vector<Assignment> Hungarian::solve(
    const std::vector<std::vector<double>>& costMatrix
) {
    int n = static_cast<int>(costMatrix.size());     // crews
    int m = n > 0 ? static_cast<int>(costMatrix[0].size()) : 0; // sites

    if (n == 0 || m == 0) return {};

    // Pad to square matrix
    int size = std::max(n, m);
    std::vector<std::vector<double>> matrix(size, std::vector<double>(size, 1e9));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            matrix[i][j] = costMatrix[i][j];
        }
    }

    std::vector<double> u(size, 0), v(size, 0);
    std::vector<int> p(size, 0), way(size, 0);

    for (int i = 1; i < size; i++) {
        p[0] = i;
        int j0 = 0;
        std::vector<double> minv(size, 1e9);
        std::vector<bool> used(size, false);

        do {
            used[j0] = true;
            int i0 = p[j0];
            double delta = 1e9;
            int j1 = 0;

            for (int j = 1; j < size; j++) {
                if (!used[j]) {
                    double cur = matrix[i0 - 1][j - 1] - u[i0] - v[j];
                    if (cur < minv[j]) {
                        minv[j] = cur;
                        way[j] = j0;
                    }
                    if (minv[j] < delta) {
                        delta = minv[j];
                        j1 = j;
                    }
                }
            }

            for (int j = 0; j < size; j++) {
                if (used[j]) {
                    u[p[j]] += delta;
                    v[j] -= delta;
                } else {
                    minv[j] -= delta;
                }
            }
            j0 = j1;
        } while (p[j0] != 0);

        do {
            int j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0 != 0);
    }

    // Extract assignments
    std::vector<Assignment> result;
    std::vector<int> assignedToSite(size, -1);

    for (int j = 1; j < size; j++) {
        if (p[j] > 0) {
            assignedToSite[p[j]] = j;
        }
    }

    for (int i = 1; i <= n; i++) {
        int siteIdx = assignedToSite[i] - 1;
        if (siteIdx >= 0 && siteIdx < m) {
            double cost = matrix[i - 1][siteIdx];
            result.push_back({i - 1, siteIdx, cost});
        }
    }

    return result;
}

double Hungarian::totalCost(const std::vector<Assignment>& assignments) {
    double total = 0.0;
    for (const auto& a : assignments) {
        total += a.cost;
    }
    return total;
}

} // namespace gridpulse