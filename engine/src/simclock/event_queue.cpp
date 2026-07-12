#include "simclock/event_queue.hpp"
#include <stdexcept>

namespace gridpulse {

void EventQueue::schedule(const SimEvent& event) {
    queue_.push(event);
}

void EventQueue::schedule(SimTime time, EventType type, int targetId,
                           double value, const std::string& desc, int priority) {
    queue_.push({time, type, targetId, value, desc, priority});
}

const SimEvent& EventQueue::peek() const {
    if (queue_.empty()) {
        throw std::runtime_error("Event queue is empty");
    }
    return queue_.top();
}

SimEvent EventQueue::pop() {
    if (queue_.empty()) {
        throw std::runtime_error("Event queue is empty");
    }
    SimEvent event = queue_.top();
    queue_.pop();
    currentTime_ = event.timestamp;
    return event;
}

bool EventQueue::isEmpty() const {
    return queue_.empty();
}

size_t EventQueue::size() const {
    return queue_.size();
}

SimTime EventQueue::currentTime() const {
    return currentTime_;
}

void EventQueue::setCurrentTime(SimTime time) {
    currentTime_ = time;
}

void EventQueue::clear() {
    while (!queue_.empty()) {
        queue_.pop();
    }
    currentTime_ = 0.0;
}

} // namespace gridpulse