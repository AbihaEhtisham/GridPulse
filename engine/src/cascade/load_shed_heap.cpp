#include "cascade/load_shed_heap.hpp"

namespace gridpulse {

void LoadShedHeap::build(const std::vector<int>& loadNodeIds, const Grid& grid) {
    clear();
    for (int id : loadNodeIds) {
        const Node& node = grid.getNode(id);
        if (node.type == NodeType::LOAD && node.currentLoadMW > 0) {
            heap_.push({id, node.tier, node.currentLoadMW});
        }
    }
}

ShedEntry LoadShedHeap::pop() {
    ShedEntry top = heap_.top();
    heap_.pop();
    return top;
}

bool LoadShedHeap::isEmpty() const {
    return heap_.empty();
}

size_t LoadShedHeap::size() const {
    return heap_.size();
}

void LoadShedHeap::clear() {
    while (!heap_.empty()) {
        heap_.pop();
    }
}

} // namespace gridpulse