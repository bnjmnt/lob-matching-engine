#include "lob/order_book.h"

#include <gtest/gtest.h>

#include <cstdint>

#include "lob/order.h"
#include "lob/trade.h"

namespace lob {
namespace {

TEST(OrderBookTest, NewBookHasNoBestBidOrAsk) {
  OrderBook book(16);

  EXPECT_FALSE(book.BestBid().has_value());
  EXPECT_FALSE(book.BestAsk().has_value());
}

TEST(OrderBookTest, AddOrderReturnsAnId) {
  OrderBook book(16);

  auto result = book.AddOrder(10000, 100, Side::kBid);

  ASSERT_TRUE(result.has_value());
}

TEST(OrderBookTest, AddOrderIdsAreUniqueAndIncreasing) {
  OrderBook book(16);

  auto first = book.AddOrder(10000, 100, Side::kBid);
  auto second = book.AddOrder(10000, 100, Side::kBid);

  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(second.has_value());
  EXPECT_NE(first->order_id, second->order_id);
  EXPECT_GT(second->order_id, first->order_id);
}

TEST(OrderBookTest, AddOrderNeverIssuesIdZero) {
  OrderBook book(16);

  auto result = book.AddOrder(10000, 100, Side::kBid);

  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->order_id, 0u);
}

TEST(OrderBookTest, AddOrderThatDoesNotCrossRestsWithNoTrades) {
  OrderBook book(16);

  auto result = book.AddOrder(10000, 100, Side::kBid);

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->rested);
  EXPECT_TRUE(result->trades.empty());
}

TEST(OrderBookTest, AddingBidSetsBestBid) {
  OrderBook book(16);

  book.AddOrder(10000, 100, Side::kBid);

  ASSERT_TRUE(book.BestBid().has_value());
  EXPECT_EQ(*book.BestBid(), 10000u);
}

TEST(OrderBookTest, AddingAskSetsBestAsk) {
  OrderBook book(16);

  book.AddOrder(10050, 100, Side::kAsk);

  ASSERT_TRUE(book.BestAsk().has_value());
  EXPECT_EQ(*book.BestAsk(), 10050u);
}

TEST(OrderBookTest, BestBidIsHighestBidPrice) {
  OrderBook book(16);

  book.AddOrder(9900, 100, Side::kBid);
  book.AddOrder(10000, 100, Side::kBid);
  book.AddOrder(9950, 100, Side::kBid);

  ASSERT_TRUE(book.BestBid().has_value());
  EXPECT_EQ(*book.BestBid(), 10000u);
}

TEST(OrderBookTest, BestAskIsLowestAskPrice) {
  OrderBook book(16);

  book.AddOrder(10100, 100, Side::kAsk);
  book.AddOrder(10050, 100, Side::kAsk);
  book.AddOrder(10075, 100, Side::kAsk);

  ASSERT_TRUE(book.BestAsk().has_value());
  EXPECT_EQ(*book.BestAsk(), 10050u);
}

TEST(OrderBookTest, BidsAndAsksDoNotInterfereWithEachOther) {
  OrderBook book(16);

  book.AddOrder(9900, 100, Side::kBid);
  book.AddOrder(10100, 100, Side::kAsk);

  ASSERT_TRUE(book.BestBid().has_value());
  ASSERT_TRUE(book.BestAsk().has_value());
  EXPECT_EQ(*book.BestBid(), 9900u);
  EXPECT_EQ(*book.BestAsk(), 10100u);
}

TEST(OrderBookTest, MultipleOrdersAtSamePriceStayAtThatBestPrice) {
  OrderBook book(16);

  book.AddOrder(10000, 100, Side::kBid);
  book.AddOrder(10000, 200, Side::kBid);

  ASSERT_TRUE(book.BestBid().has_value());
  EXPECT_EQ(*book.BestBid(), 10000u);
}

TEST(OrderBookTest, CancelOrderOnUnknownIdReturnsFalse) {
  OrderBook book(16);

  EXPECT_FALSE(book.CancelOrder(999));
}

TEST(OrderBookTest, CancelOrderOnKnownIdReturnsTrue) {
  OrderBook book(16);
  auto result = book.AddOrder(10000, 100, Side::kBid);
  ASSERT_TRUE(result.has_value());

  EXPECT_TRUE(book.CancelOrder(result->order_id));
}

TEST(OrderBookTest, CancelOrderTwiceReturnsFalseOnSecondCall) {
  OrderBook book(16);
  auto result = book.AddOrder(10000, 100, Side::kBid);
  ASSERT_TRUE(result.has_value());

  book.CancelOrder(result->order_id);

  EXPECT_FALSE(book.CancelOrder(result->order_id));
}

TEST(OrderBookTest, CancelingOnlyOrderAtPriceRemovesThatBestPrice) {
  OrderBook book(16);
  auto result = book.AddOrder(10000, 100, Side::kBid);
  ASSERT_TRUE(result.has_value());

  book.CancelOrder(result->order_id);

  EXPECT_FALSE(book.BestBid().has_value());
}

TEST(OrderBookTest, CancelingBestPriceRevealsNextBestPrice) {
  OrderBook book(16);
  auto first = book.AddOrder(10000, 100, Side::kBid);
  book.AddOrder(9900, 100, Side::kBid);
  ASSERT_TRUE(first.has_value());

  book.CancelOrder(first->order_id);

  ASSERT_TRUE(book.BestBid().has_value());
  EXPECT_EQ(*book.BestBid(), 9900u);
}

TEST(OrderBookTest, CancelingNonBestPriceLeavesBestPriceUnchanged) {
  OrderBook book(16);
  book.AddOrder(10000, 100, Side::kBid);
  auto worse = book.AddOrder(9900, 100, Side::kBid);
  ASSERT_TRUE(worse.has_value());

  book.CancelOrder(worse->order_id);

  ASSERT_TRUE(book.BestBid().has_value());
  EXPECT_EQ(*book.BestBid(), 10000u);
}

TEST(OrderBookTest, CancelingOneOfTwoOrdersAtSamePriceKeepsPriceLevel) {
  OrderBook book(16);
  auto first = book.AddOrder(10000, 100, Side::kBid);
  book.AddOrder(10000, 200, Side::kBid);
  ASSERT_TRUE(first.has_value());

  book.CancelOrder(first->order_id);

  ASSERT_TRUE(book.BestBid().has_value());
  EXPECT_EQ(*book.BestBid(), 10000u);
}

TEST(OrderBookTest, AddOrderFailsWhenPoolIsExhausted) {
  OrderBook book(2);
  book.AddOrder(10000, 100, Side::kBid);
  book.AddOrder(10001, 100, Side::kBid);

  auto result = book.AddOrder(10002, 100, Side::kBid);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().error, AddOrderError::kPoolExhausted);
}

TEST(OrderBookTest, PoolExhaustionErrorCarriesNoTradesWhenNothingCrossed) {
  OrderBook book(2);
  book.AddOrder(10000, 100, Side::kBid);
  book.AddOrder(10001, 100, Side::kBid);

  auto result = book.AddOrder(10002, 100, Side::kBid);

  ASSERT_FALSE(result.has_value());
  EXPECT_TRUE(result.error().trades.empty());
}

TEST(OrderBookTest, CancelFreesPoolCapacityForReuse) {
  OrderBook book(1);
  auto result = book.AddOrder(10000, 100, Side::kBid);
  ASSERT_TRUE(result.has_value());
  ASSERT_FALSE(book.AddOrder(10001, 100, Side::kBid).has_value());

  book.CancelOrder(result->order_id);
  auto reused = book.AddOrder(10002, 100, Side::kBid);

  EXPECT_TRUE(reused.has_value());
}

TEST(OrderBookTest, CrossingBidExactlyFillsRestingAskAndDoesNotRest) {
  OrderBook book(16);
  auto ask = book.AddOrder(10000, 50, Side::kAsk);
  ASSERT_TRUE(ask.has_value());

  auto bid = book.AddOrder(10000, 50, Side::kBid);

  ASSERT_TRUE(bid.has_value());
  EXPECT_FALSE(bid->rested);
  ASSERT_EQ(bid->trades.size(), 1u);
  EXPECT_EQ(bid->trades[0].resting_order_id, ask->order_id);
  EXPECT_EQ(bid->trades[0].incoming_order_id, bid->order_id);
  EXPECT_EQ(bid->trades[0].quantity, 50u);
}

TEST(OrderBookTest, TradePriceIsTheRestingOrdersPriceNotTheIncomingPrice) {
  OrderBook book(16);
  book.AddOrder(10000, 50, Side::kAsk);

  // Incoming bid is willing to pay more than the resting ask; the trade
  // should print at the resting (better) price, not the incoming price.
  auto bid = book.AddOrder(10050, 50, Side::kBid);

  ASSERT_TRUE(bid.has_value());
  ASSERT_EQ(bid->trades.size(), 1u);
  EXPECT_EQ(bid->trades[0].price_ticks, 10000u);
}

TEST(OrderBookTest, NonCrossingOrdersOnBothSidesRestWithNoTrades) {
  OrderBook book(16);
  book.AddOrder(10000, 50, Side::kAsk);

  auto bid = book.AddOrder(9900, 50, Side::kBid);

  ASSERT_TRUE(bid.has_value());
  EXPECT_TRUE(bid->rested);
  EXPECT_TRUE(bid->trades.empty());
}

TEST(OrderBookTest, IncomingOrderLargerThanRestingPartiallyFillsAndRests) {
  OrderBook book(16);
  auto ask = book.AddOrder(10000, 20, Side::kAsk);
  ASSERT_TRUE(ask.has_value());

  auto bid = book.AddOrder(10000, 50, Side::kBid);

  ASSERT_TRUE(bid.has_value());
  EXPECT_TRUE(bid->rested);
  ASSERT_EQ(bid->trades.size(), 1u);
  EXPECT_EQ(bid->trades[0].quantity, 20u);
  ASSERT_TRUE(book.BestBid().has_value());
  EXPECT_EQ(*book.BestBid(), 10000u);
}

TEST(OrderBookTest, IncomingOrderSmallerThanRestingPartiallyFillsRestingOnly) {
  OrderBook book(16);
  auto ask = book.AddOrder(10000, 20, Side::kAsk);
  ASSERT_TRUE(ask.has_value());

  auto bid = book.AddOrder(10000, 5, Side::kBid);

  ASSERT_TRUE(bid.has_value());
  EXPECT_FALSE(bid->rested);
  ASSERT_EQ(bid->trades.size(), 1u);
  EXPECT_EQ(bid->trades[0].quantity, 5u);
  // The partially filled ask should still be resting at the same price.
  ASSERT_TRUE(book.BestAsk().has_value());
  EXPECT_EQ(*book.BestAsk(), 10000u);
}

TEST(OrderBookTest, FullyFilledRestingOrderCanNoLongerBeCancelled) {
  OrderBook book(16);
  auto ask = book.AddOrder(10000, 50, Side::kAsk);
  ASSERT_TRUE(ask.has_value());

  book.AddOrder(10000, 50, Side::kBid);

  EXPECT_FALSE(book.CancelOrder(ask->order_id));
}

TEST(OrderBookTest, PartiallyFilledRestingOrderCanStillBeCancelled) {
  OrderBook book(16);
  auto ask = book.AddOrder(10000, 20, Side::kAsk);
  ASSERT_TRUE(ask.has_value());

  book.AddOrder(10000, 5, Side::kBid);

  EXPECT_TRUE(book.CancelOrder(ask->order_id));
  EXPECT_FALSE(book.BestAsk().has_value());
}

TEST(OrderBookTest, IncomingOrderSweepsMultipleAsksAtSamePriceInFifoOrder) {
  OrderBook book(16);
  auto first = book.AddOrder(10000, 10, Side::kAsk);
  auto second = book.AddOrder(10000, 10, Side::kAsk);
  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(second.has_value());

  auto bid = book.AddOrder(10000, 15, Side::kBid);

  ASSERT_TRUE(bid.has_value());
  ASSERT_EQ(bid->trades.size(), 2u);
  EXPECT_EQ(bid->trades[0].resting_order_id, first->order_id);
  EXPECT_EQ(bid->trades[0].quantity, 10u);
  EXPECT_EQ(bid->trades[1].resting_order_id, second->order_id);
  EXPECT_EQ(bid->trades[1].quantity, 5u);
}

TEST(OrderBookTest, IncomingOrderSweepsMultiplePriceLevels) {
  OrderBook book(16);
  book.AddOrder(10000, 10, Side::kAsk);
  book.AddOrder(10050, 10, Side::kAsk);

  auto bid = book.AddOrder(10050, 20, Side::kBid);

  ASSERT_TRUE(bid.has_value());
  ASSERT_EQ(bid->trades.size(), 2u);
  EXPECT_EQ(bid->trades[0].price_ticks, 10000u);
  EXPECT_EQ(bid->trades[1].price_ticks, 10050u);
  EXPECT_FALSE(book.BestAsk().has_value());
}

TEST(OrderBookTest, SweepStopsOncePriceNoLongerCrosses) {
  OrderBook book(16);
  book.AddOrder(10000, 10, Side::kAsk);
  book.AddOrder(10100, 10, Side::kAsk);

  // Willing to pay up to 10050, so only the 10000 level should trade.
  auto bid = book.AddOrder(10050, 20, Side::kBid);

  ASSERT_TRUE(bid.has_value());
  ASSERT_EQ(bid->trades.size(), 1u);
  EXPECT_EQ(bid->trades[0].price_ticks, 10000u);
  EXPECT_TRUE(bid->rested);
  ASSERT_TRUE(book.BestAsk().has_value());
  EXPECT_EQ(*book.BestAsk(), 10100u);
}

TEST(OrderBookTest, PartiallyFilledRestingOrderRetainsTimePriority) {
  // Regression test for PushFront requeuing: a resting order that's
  // only partially filled must stay ahead of an order that arrived
  // after it, not get pushed to the back of its price level.
  OrderBook book(16);
  auto first = book.AddOrder(10000, 10, Side::kAsk);
  auto second = book.AddOrder(10000, 5, Side::kAsk);
  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(second.has_value());

  book.AddOrder(10000, 4, Side::kBid);  // partially fills `first` by 4

  auto sweeping_bid = book.AddOrder(10000, 20, Side::kBid);

  ASSERT_TRUE(sweeping_bid.has_value());
  ASSERT_EQ(sweeping_bid->trades.size(), 2u);
  EXPECT_EQ(sweeping_bid->trades[0].resting_order_id, first->order_id);
  EXPECT_EQ(sweeping_bid->trades[0].quantity, 6u);
  EXPECT_EQ(sweeping_bid->trades[1].resting_order_id, second->order_id);
  EXPECT_EQ(sweeping_bid->trades[1].quantity, 5u);
}

TEST(OrderBookTest, CrossingAskMatchesAgainstRestingBid) {
  // Mirror of the bid-crosses-ask tests, exercising the asks_-side
  // instantiation of MatchAgainst.
  OrderBook book(16);
  auto bid = book.AddOrder(10000, 50, Side::kBid);
  ASSERT_TRUE(bid.has_value());

  auto ask = book.AddOrder(10000, 50, Side::kAsk);

  ASSERT_TRUE(ask.has_value());
  EXPECT_FALSE(ask->rested);
  ASSERT_EQ(ask->trades.size(), 1u);
  EXPECT_EQ(ask->trades[0].resting_order_id, bid->order_id);
  EXPECT_EQ(ask->trades[0].quantity, 50u);
}

}  // namespace
}  // namespace lob