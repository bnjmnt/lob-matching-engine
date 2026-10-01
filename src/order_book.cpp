#include "lob/order_book.h"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <vector>

#include "lob/order.h"
#include "lob/order_pool.h"
#include "lob/price_level.h"
#include "lob/trade.h"

namespace lob {

OrderBook::OrderBook(std::uint64_t max_orders) : pool_(max_orders) {}

std::expected<AddOrderResult, AddOrderFailure> OrderBook::AddOrder(
    std::uint64_t price_ticks, std::uint32_t quantity, Side side) {
  bool is_bid = (side == Side::kBid);
  std::uint64_t incoming_id = NextOrderId();
  std::vector<Trade> trades;

  std::uint32_t remaining = is_bid
                                ? MatchAgainst(asks_, incoming_id, price_ticks,
                                               is_bid, quantity, trades)
                                : MatchAgainst(bids_, incoming_id, price_ticks,
                                               is_bid, quantity, trades);

  bool rested = false;
  if (remaining > 0) {
    if (!pool_.GetFreeCount()) {
      return std::unexpected(
          AddOrderFailure{AddOrderError::kPoolExhausted, std::move(trades)});
    }

    Order* order = pool_.Acquire();
    order->id = incoming_id;
    order->price_ticks = price_ticks;
    order->remaining_quantity = quantity;
    order->side = side;

    if (is_bid) {
      auto [it, is_inserted] = bids_.try_emplace(price_ticks, nullptr);
      if (is_inserted) {
        it->second = std::make_unique<PriceLevel>(price_ticks);
      }
      it->second->PushBack(order);
    } else {
      auto [it, is_inserted] = asks_.try_emplace(price_ticks, nullptr);
      if (is_inserted) {
        it->second = std::make_unique<PriceLevel>(price_ticks);
      }
      it->second->PushBack(order);
    }

    order_lookup_.emplace(order->id, order);
    rested = true;
  }

  return AddOrderResult{incoming_id, std::move(trades), rested};
}

bool OrderBook::CancelOrder(std::uint64_t order_id) {
  auto it = order_lookup_.find(order_id);
  if (it == order_lookup_.end()) {
    return false;
  }
  Order* order = it->second;
  bool is_level_empty = order->level->Remove(order);
  if (is_level_empty) {
    if (order->side == Side::kAsk) {
      asks_.erase(order->price_ticks);
    } else {
      bids_.erase(order->price_ticks);
    }
  }
  order_lookup_.erase(it);
  pool_.Release(order);
  return true;
}

std::optional<std::uint64_t> OrderBook::BestBid() const {
  if (bids_.empty()) {
    return std::nullopt;
  }
  return bids_.begin()->first;
}

std::optional<std::uint64_t> OrderBook::BestAsk() const {
  if (asks_.empty()) {
    return std::nullopt;
  }
  return asks_.begin()->first;
}

template <typename PriceLevelMap>
std::uint32_t OrderBook::MatchAgainst(PriceLevelMap& opposite_side,
                                      std::uint64_t incoming_id,
                                      std::uint64_t price_ticks, bool is_bid,
                                      std::uint32_t quantity,
                                      std::vector<Trade>& trades) {
  std::uint32_t remaining = quantity;

  while (remaining > 0 && !opposite_side.empty()) {
    std::uint64_t best_price = opposite_side.begin()->first;
    bool crosses =
        is_bid ? (price_ticks >= best_price) : (price_ticks <= best_price);
    if (!crosses) {
      break;
    }

    PriceLevel* level = opposite_side.begin()->second.get();
    Order* resting = level->PopFront();

    std::uint32_t trade_qty = std::min(remaining, resting->remaining_quantity);
    remaining -= trade_qty;
    resting->remaining_quantity -= trade_qty;

    trades.push_back(
        Trade{resting->id, incoming_id, level->GetPriceTicks(), trade_qty});

    if (resting->remaining_quantity > 0) {
      level->PushFront(resting);
    } else {
      order_lookup_.erase(resting->id);
      pool_.Release(resting);
    }

    if (level->IsEmpty()) {
      opposite_side.erase(opposite_side.begin());
    }
  }

  return remaining;
}

std::uint64_t OrderBook::NextOrderId() { return next_order_id_++; }

}  // namespace lob