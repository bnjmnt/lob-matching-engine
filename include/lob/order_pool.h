#ifndef LOB_MATCHING_ENGINE_INCLUDE_LOB_ORDER_POOL_H_
#define LOB_MATCHING_ENGINE_INCLUDE_LOB_ORDER_POOL_H_

#include <cstddef>
#include <vector>

#include "lob/order.h"

namespace lob {

class OrderPool {
 public:
  explicit OrderPool(std::size_t capacity);

  Order* Acquire();
  void Release(Order* order);

  std::size_t GetCapacity() const;
  std::size_t GetFreeCount() const;

 private:
  std::vector<Order> storage_;
  std::vector<Order*> free_list_;
};

}  // namespace lob

#endif  // LOB_MATCHING_ENGINE_INCLUDE_LOB_ORDER_POOL_H_
