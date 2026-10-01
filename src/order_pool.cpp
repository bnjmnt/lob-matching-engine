#include "lob/order_pool.h"

#include <cstddef>

namespace lob {

OrderPool::OrderPool(std::size_t capacity) : storage_(capacity) {
  free_list_.reserve(capacity);
  for (Order& order : storage_) {
    free_list_.push_back(&order);
  }
}

Order* OrderPool::Acquire() {
  if (free_list_.empty()) {
    return nullptr;
  }
  Order* order = free_list_.back();
  free_list_.pop_back();
  return order;
}

void OrderPool::Release(Order* order) {
  *order = Order{};
  free_list_.push_back(order);
}

std::size_t OrderPool::GetCapacity() const { return storage_.size(); }

std::size_t OrderPool::GetFreeCount() const { return free_list_.size(); }

}  // namespace lob
