#include "lob/order_pool.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <unordered_set>

#include "lob/order.h"

namespace lob {
namespace {

TEST(OrderPoolTest, CapacityMatchesConstructorArgument) {
  OrderPool pool(16);

  EXPECT_EQ(pool.GetCapacity(), 16u);
  EXPECT_EQ(pool.GetFreeCount(), 16u);
}

TEST(OrderPoolTest, AcquireReturnsNonNullAndDecrementsFreeCount) {
  OrderPool pool(4);

  Order* order = pool.Acquire();

  ASSERT_NE(order, nullptr);
  EXPECT_EQ(pool.GetFreeCount(), 3u);
}

TEST(OrderPoolTest, AcquireReturnsDistinctPointersEachTime) {
  OrderPool pool(4);
  std::unordered_set<Order*> seen;

  for (int i = 0; i < 4; ++i) {
    Order* order = pool.Acquire();
    ASSERT_NE(order, nullptr);
    EXPECT_TRUE(seen.insert(order).second)
        << "Acquire returned a duplicate pointer";
  }
}

TEST(OrderPoolTest, AcquireOnExhaustedPoolReturnsNullptr) {
  OrderPool pool(2);

  pool.Acquire();
  pool.Acquire();

  EXPECT_EQ(pool.GetFreeCount(), 0u);
  EXPECT_EQ(pool.Acquire(), nullptr);
}

TEST(OrderPoolTest, ReleaseIncrementsFreeCount) {
  OrderPool pool(2);
  Order* order = pool.Acquire();

  pool.Release(order);

  EXPECT_EQ(pool.GetFreeCount(), 2u);
}

TEST(OrderPoolTest, ReleaseResetsOrderToDefaultState) {
  OrderPool pool(1);
  Order* order = pool.Acquire();
  order->id = 99;
  order->price_ticks = 12345;
  order->remaining_quantity = 500;
  order->side = Side::kAsk;

  pool.Release(order);
  Order* reacquired = pool.Acquire();

  ASSERT_EQ(reacquired, order);
  EXPECT_EQ(reacquired->id, 0u);
  EXPECT_EQ(reacquired->price_ticks, 0u);
  EXPECT_EQ(reacquired->remaining_quantity, 0u);
  EXPECT_EQ(reacquired->side, Side::kBid);
}

TEST(OrderPoolTest, AcquireReleaseAcquireCycleReusesCapacity) {
  OrderPool pool(2);

  Order* first = pool.Acquire();
  Order* second = pool.Acquire();
  pool.Release(first);
  Order* third = pool.Acquire();

  EXPECT_EQ(third, first);
  EXPECT_NE(second, third);
  EXPECT_EQ(pool.GetFreeCount(), 0u);
}

TEST(OrderPoolTest, ZeroCapacityPoolAlwaysReturnsNullptr) {
  OrderPool pool(0);

  EXPECT_EQ(pool.GetCapacity(), 0u);
  EXPECT_EQ(pool.Acquire(), nullptr);
}

}  // namespace
}  // namespace lob