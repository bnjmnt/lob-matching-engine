#include "lob/price_level.h"

#include <cstdint>

#include "lob/order.h"

namespace lob {

PriceLevel::PriceLevel(std::uint64_t price_ticks)
    : price_ticks_(price_ticks),
      total_quantity_(0),
      head_(nullptr),
      tail_(nullptr) {}

void PriceLevel::PushBack(Order* order) {
  if (IsEmpty()) {
    head_ = order;
  } else {
    tail_->next = order;
    order->prev = tail_;
  }
  tail_ = order;
  order->next = nullptr;
  order->level = this;
  total_quantity_ += order->remaining_quantity;
}

void PriceLevel::PushFront(Order* order) {
  if (IsEmpty()) {
    tail_ = order;
  } else {
    head_->prev = order;
    order->next = head_;
  }
  head_ = order;
  order->prev = nullptr;
  order->level = this;
  total_quantity_ += order->remaining_quantity;
}

Order* PriceLevel::PopFront() {
  Order* order = head_;
  head_ = head_->next;
  if (head_) {
    head_->prev = nullptr;
  } else {
    tail_ = nullptr;
  }
  order->next = nullptr;
  order->level = nullptr;
  total_quantity_ -= order->remaining_quantity;
  return order;
}

bool PriceLevel::Remove(Order* order) {
  if (order->prev) {
    order->prev->next = order->next;
  } else {
    head_ = head_->next;
  }
  if (order->next) {
    order->next->prev = order->prev;
  } else {
    tail_ = order->prev;
  }
  order->level = nullptr;
  total_quantity_ -= order->remaining_quantity;
  return IsEmpty();
}

bool PriceLevel::IsEmpty() const { return head_ == nullptr; }
std::uint64_t PriceLevel::GetPriceTicks() const { return price_ticks_; }
std::uint64_t PriceLevel::GetTotalQuantity() const { return total_quantity_; }

}  // namespace lob
