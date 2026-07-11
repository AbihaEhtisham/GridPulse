#pragma once

#include <vector>
#include <set>

namespace gridpulse {

class UnionFind {
public:
    // Initialize with n elements (0 to n-1), each in its own set
    explicit UnionFind(int n);

    // Find the representative (root) of the set containing x
    // Uses path compression
    int find(int x);

    // Merge the sets containing x and y
    // Uses union by rank
    // Returns true if a merge happened, false if already same set
    bool unionSets(int x, int y);

    // Check if x and y are in the same set
    bool connected(int x, int y);

    // Get all distinct sets as lists of element IDs
    // Each inner vector is one connected component
    std::vector<std::vector<int>> getComponents();

    // Number of distinct sets
    int componentCount() const;

    // Reset the structure (all elements isolated)
    void reset();

private:
    std::vector<int> parent_;  // parent[i] = parent of i, or i if root
    std::vector<int> rank_;    // rank[i] = approximate depth of tree rooted at i
    int count_;                // number of distinct components
};

} // namespace gridpulse