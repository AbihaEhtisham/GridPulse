#pragma once

#include <vector>
#include <queue>
#include "graph/grid.hpp"

namespace gridpulse {

struct ShedEntry {
    int nodeId;
    CriticalityTier tier;
    double loadMW;

    // Priority: lower tier number = more critical = shed LAST
    // For same tier: higher load = shed FIRST
    bool operator<(const ShedEntry& other) const {
        if (tier != other.tier) {
            // Higher tier value = less critical = higher priority to shed
            return static_cast<int>(tier) < static_cast<int>(other.tier);
        }
        // Same tier: shed larger loads first
        return loadMW < other.loadMW;
    }
};

class LoadShedHeap {
public:
    LoadShedHeap() = default;

    // Build heap from load nodes in a grid
    void build(const std::vector<int>& loadNodeIds, const Grid& grid);

    // Get the next load to shed (least critical, highest demand)
    ShedEntry pop();

    // Check if heap is empty
    bool isEmpty() const;

    // Get heap size
    size_t size() const;

    // Clear the heap
    void clear();

private:
    std::priority_queue<ShedEntry> heap_;
};

} // namespace gridpulse