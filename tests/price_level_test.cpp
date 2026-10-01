#include "lob/price_level.h"

#include <gtest/gtest.h>

#include "lob/order.h"

namespace lob {
namespace {

TEST(PriceLevelTest, ConstructorSetsPriceAndStartsEmpty) {
  PriceLevel level(10050);

  EXPECT_EQ(level.GetPriceTicks(), 10050u);
  EXPECT_EQ(level.GetTotalQuantity(), 0u);
  EXPECT_TRUE(level.IsEmpty());
}

TEST(PriceLevelTest, PushBackOnEmptyLevelIsNotEmpty) {
  PriceLevel level(100);
  Order order;
  order.id = 1;
  order.remaining_quantity = 50;

  level.PushBack(&order);

  EXPECT_FALSE(level.IsEmpty());
}

TEST(PriceLevelTest, PushBackAccumulatesTotalQuantity) {
  PriceLevel level(100);
  Order first;
  first.remaining_quantity = 50;
  Order second;
  second.remaining_quantity = 30;

  level.PushBack(&first);
  level.PushBack(&second);

  EXPECT_EQ(level.GetTotalQuantity(), 80u);
}

TEST(PriceLevelTest, PopFrontReturnsOrdersInFifoOrder) {
  PriceLevel level(100);
  Order first;
  first.id = 1;
  first.remaining_quantity = 10;
  Order second;
  second.id = 2;
  second.remaining_quantity = 20;
  Order third;
  third.id = 3;
  third.remaining_quantity = 30;

  level.PushBack(&first);
  level.PushBack(&second);
  level.PushBack(&third);

  EXPECT_EQ(level.PopFront(), &first);
  EXPECT_EQ(level.PopFront(), &second);
  EXPECT_EQ(level.PopFront(), &third);
}

TEST(PriceLevelTest, PopFrontDecrementsTotalQuantity) {
  PriceLevel level(100);
  Order first;
  first.remaining_quantity = 10;
  Order second;
  second.remaining_quantity = 20;
  level.PushBack(&first);
  level.PushBack(&second);

  level.PopFront();

  EXPECT_EQ(level.GetTotalQuantity(), 20u);
}

TEST(PriceLevelTest, PopFrontOnSingleElementLevelEmptiesLevel) {
  PriceLevel level(100);
  Order order;
  order.remaining_quantity = 10;
  level.PushBack(&order);

  Order* popped = level.PopFront();

  EXPECT_EQ(popped, &order);
  EXPECT_TRUE(level.IsEmpty());
  EXPECT_EQ(level.GetTotalQuantity(), 0u);
}

TEST(PriceLevelTest, PopFrontThenPushBackRelinksTailCorrectly) {
  // Regression test: after popping down to empty, tail_ must not be left
  // dangling, or the next PushBack corrupts the list.
  PriceLevel level(100);
  Order first;
  first.remaining_quantity = 10;
  level.PushBack(&first);
  level.PopFront();

  Order second;
  second.remaining_quantity = 15;
  level.PushBack(&second);

  EXPECT_FALSE(level.IsEmpty());
  EXPECT_EQ(level.PopFront(), &second);
}

TEST(PriceLevelTest, RemoveHeadRelinksRemainingOrders) {
  PriceLevel level(100);
  Order first;
  first.remaining_quantity = 10;
  Order second;
  second.remaining_quantity = 20;
  level.PushBack(&first);
  level.PushBack(&second);

  bool now_empty = level.Remove(&first);

  EXPECT_FALSE(now_empty);
  EXPECT_EQ(level.PopFront(), &second);
}

TEST(PriceLevelTest, RemoveTailRelinksRemainingOrders) {
  PriceLevel level(100);
  Order first;
  first.remaining_quantity = 10;
  Order second;
  second.remaining_quantity = 20;
  level.PushBack(&first);
  level.PushBack(&second);

  level.Remove(&second);

  EXPECT_EQ(level.PopFront(), &first);
  EXPECT_TRUE(level.IsEmpty());
}

TEST(PriceLevelTest, RemoveMiddleRelinksNeighbors) {
  PriceLevel level(100);
  Order first;
  first.remaining_quantity = 10;
  Order second;
  second.remaining_quantity = 20;
  Order third;
  third.remaining_quantity = 30;
  level.PushBack(&first);
  level.PushBack(&second);
  level.PushBack(&third);

  level.Remove(&second);

  EXPECT_EQ(level.PopFront(), &first);
  EXPECT_EQ(level.PopFront(), &third);
}

TEST(PriceLevelTest, RemoveOnlyOrderEmptiesLevel) {
  PriceLevel level(100);
  Order order;
  order.remaining_quantity = 10;
  level.PushBack(&order);

  bool now_empty = level.Remove(&order);

  EXPECT_TRUE(now_empty);
  EXPECT_TRUE(level.IsEmpty());
}

TEST(PriceLevelTest, RemoveDecrementsTotalQuantity) {
  PriceLevel level(100);
  Order first;
  first.remaining_quantity = 10;
  Order second;
  second.remaining_quantity = 20;
  level.PushBack(&first);
  level.PushBack(&second);

  level.Remove(&first);

  EXPECT_EQ(level.GetTotalQuantity(), 20u);
}

TEST(PriceLevelTest, PushBackSetsOrderLevelBackPointer) {
  PriceLevel level(100);
  Order order;
  order.remaining_quantity = 10;

  level.PushBack(&order);

  EXPECT_EQ(order.level, &level);
}

TEST(PriceLevelTest, PushBackSetsBackPointerOnEveryOrder) {
  PriceLevel level(100);
  Order first;
  first.remaining_quantity = 10;
  Order second;
  second.remaining_quantity = 20;

  level.PushBack(&first);
  level.PushBack(&second);

  EXPECT_EQ(first.level, &level);
  EXPECT_EQ(second.level, &level);
}

TEST(PriceLevelTest, PopFrontClearsOrderLevelBackPointer) {
  PriceLevel level(100);
  Order order;
  order.remaining_quantity = 10;
  level.PushBack(&order);

  Order* popped = level.PopFront();

  EXPECT_EQ(popped->level, nullptr);
}

TEST(PriceLevelTest, RemoveClearsOrderLevelBackPointer) {
  PriceLevel level(100);
  Order first;
  first.remaining_quantity = 10;
  Order second;
  second.remaining_quantity = 20;
  level.PushBack(&first);
  level.PushBack(&second);

  level.Remove(&first);

  EXPECT_EQ(first.level, nullptr);
}

TEST(PriceLevelTest, RemoveDoesNotClearBackPointerOfRemainingOrders) {
  PriceLevel level(100);
  Order first;
  first.remaining_quantity = 10;
  Order second;
  second.remaining_quantity = 20;
  level.PushBack(&first);
  level.PushBack(&second);

  level.Remove(&first);

  EXPECT_EQ(second.level, &level);
}

TEST(PriceLevelTest, PushFrontOnEmptyLevelIsNotEmpty) {
  PriceLevel level(100);
  Order order;
  order.remaining_quantity = 50;

  level.PushFront(&order);

  EXPECT_FALSE(level.IsEmpty());
}

TEST(PriceLevelTest, PushFrontOnEmptyLevelIsRetrievableViaPopFront) {
  PriceLevel level(100);
  Order order;
  order.remaining_quantity = 50;

  level.PushFront(&order);

  EXPECT_EQ(level.PopFront(), &order);
}

TEST(PriceLevelTest, PushFrontAccumulatesTotalQuantity) {
  PriceLevel level(100);
  Order first;
  first.remaining_quantity = 50;
  Order second;
  second.remaining_quantity = 30;

  level.PushFront(&first);
  level.PushFront(&second);

  EXPECT_EQ(level.GetTotalQuantity(), 80u);
}

TEST(PriceLevelTest, PushFrontPlacesOrderAheadOfExistingOrders) {
  // A resting order partially filled during matching goes back to the
  // front of its level, ahead of orders that arrived later, since it
  // still has time priority from its original arrival.
  PriceLevel level(100);
  Order first;
  first.remaining_quantity = 10;
  Order second;
  second.remaining_quantity = 20;
  level.PushBack(&first);
  level.PushBack(&second);

  Order requeued;
  requeued.remaining_quantity = 5;
  level.PushFront(&requeued);

  EXPECT_EQ(level.PopFront(), &requeued);
  EXPECT_EQ(level.PopFront(), &first);
  EXPECT_EQ(level.PopFront(), &second);
}

TEST(PriceLevelTest, PushFrontSetsOrderLevelBackPointer) {
  PriceLevel level(100);
  Order order;
  order.remaining_quantity = 10;

  level.PushFront(&order);

  EXPECT_EQ(order.level, &level);
}

TEST(PriceLevelTest, PushFrontThenPushBackKeepsCorrectOrder) {
  PriceLevel level(100);
  Order front_order;
  front_order.remaining_quantity = 10;
  Order back_order;
  back_order.remaining_quantity = 20;

  level.PushFront(&front_order);
  level.PushBack(&back_order);

  EXPECT_EQ(level.PopFront(), &front_order);
  EXPECT_EQ(level.PopFront(), &back_order);
}

TEST(PriceLevelTest, PushFrontUpdatesTailWhenLevelWasEmpty) {
  // Regression test: PushFront on an empty level must also set tail_,
  // or a later PushBack would append after a stale/absent tail.
  PriceLevel level(100);
  Order first;
  first.remaining_quantity = 10;
  level.PushFront(&first);

  Order second;
  second.remaining_quantity = 20;
  level.PushBack(&second);

  EXPECT_EQ(level.PopFront(), &first);
  EXPECT_EQ(level.PopFront(), &second);
}

}  // namespace
}  // namespace lob