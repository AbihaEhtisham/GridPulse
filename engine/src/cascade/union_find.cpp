#include "cascade/union_find.hpp"
#include <algorithm>

namespace gridpulse {

UnionFind::UnionFind(int n)
    : parent_(n)
    , rank_(n, 0)
    , count_(n)
{
    for (int i = 0; i < n; ++i) {
        parent_[i] = i;
    }
}

int UnionFind::find(int x) {
    if (x < 0 || x >= static_cast<int>(parent_.size())) return -1;

    // Path compression: make each node point directly to root
    if (parent_[x] != x) {
        parent_[x] = find(parent_[x]);
    }
    return parent_[x];
}

bool UnionFind::unionSets(int x, int y) {
    int rootX = find(x);
    int rootY = find(y);

    if (rootX < 0 || rootY < 0) return false;
    if (rootX == rootY) return false; // already in same set

    // Union by rank: attach smaller tree under larger tree
    if (rank_[rootX] < rank_[rootY]) {
        parent_[rootX] = rootY;
    } else if (rank_[rootX] > rank_[rootY]) {
        parent_[rootY] = rootX;
    } else {
        parent_[rootY] = rootX;
        rank_[rootX]++;
    }

    count_--;
    return true;
}

bool UnionFind::connected(int x, int y) {
    return find(x) == find(y);
}

std::vector<std::vector<int>> UnionFind::getComponents() {
    // Map root -> list of members
    std::vector<std::vector<int>> rootToMembers(parent_.size());

    for (int i = 0; i < static_cast<int>(parent_.size()); ++i) {
        int root = find(i);
        if (root >= 0) {
            rootToMembers[root].push_back(i);
        }
    }

    // Collect non-empty components
    std::vector<std::vector<int>> components;
    for (auto& members : rootToMembers) {
        if (!members.empty()) {
            components.push_back(std::move(members));
        }
    }

    return components;
}

int UnionFind::componentCount() const {
    return count_;
}

void UnionFind::reset() {
    int n = static_cast<int>(parent_.size());
    for (int i = 0; i < n; ++i) {
        parent_[i] = i;
        rank_[i] = 0;
    }
    count_ = n;
}

} // namespace gridpulse