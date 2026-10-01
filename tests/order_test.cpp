#include "lob/order.h"

#include <gtest/gtest.h>

namespace lob {
namespace {

TEST(OrderTest, DefaultConstructedFieldsAreZeroed) {
  Order order;

  EXPECT_EQ(order.id, 0u);
  EXPECT_EQ(order.price_ticks, 0u);
  EXPECT_EQ(order.next, nullptr);
  EXPECT_EQ(order.prev, nullptr);
  EXPECT_EQ(order.remaining_quantity, 0u);
  EXPECT_EQ(order.side, Side::kBid);
}

TEST(OrderTest, FieldsAreIndependentlyAssignable) {
  Order order;
  order.id = 42;
  order.price_ticks = 10050;
  order.remaining_quantity = 100;
  order.side = Side::kAsk;

  EXPECT_EQ(order.id, 42u);
  EXPECT_EQ(order.price_ticks, 10050u);
  EXPECT_EQ(order.remaining_quantity, 100u);
  EXPECT_EQ(order.side, Side::kAsk);
}

}  // namespace
}  // namespace lob