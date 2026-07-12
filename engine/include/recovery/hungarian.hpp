#pragma once

#include <vector>
#include <limits>

namespace gridpulse {

struct Assignment {
    int crewIndex;   // which crew
    int siteIndex;   // which damage site
    double cost;     // travel cost (distance)
};

class Hungarian {
public:
    // Solve the assignment problem: N crews to M damage sites
    // Returns optimal assignment minimizing total travel distance
    // Uses the Hungarian algorithm, O(N^2 * M)
    static std::vector<Assignment> solve(
        const std::vector<std::vector<double>>& costMatrix
    );

    // Get total cost of an assignment
    static double totalCost(const std::vector<Assignment>& assignments);
};

} // namespace gridpulse