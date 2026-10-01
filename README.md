# Limit Order Book Matching Engine

A C++23 limit order book matching engine.

## What it does

`OrderBook` implements a single-instrument limit order book with price-time priority matching:

- **Add orders** at a given price, quantity, and side. An incoming order matches against resting liquidity on the opposite side of the book if it crosses, walking price levels from best outward and generating trades until the incoming order is filled or no longer crosses.
- **Partial fills** are handled on both sides: a resting order that isn't fully consumed keeps its place at the front of its price level (its original time priority is preserved), and any unfilled quantity on the incoming order rests on the book.
- **Cancel orders** by ID in O(1), no scan required.
- **Best bid / best ask** lookups in O(1).

## Design decisions

A few choices worth knowing if you're reading the code:

- **Integer tick pricing.** Prices are `uint64_t` ticks, not floating point, to avoid floating-point comparison issues in matching logic.
- **Intrusive doubly-linked list for orders.** `Order` carries its own `next`/`prev` pointers, so a `PriceLevel`'s FIFO queue needs no separate node allocation.
- **`OrderPool`, a free-list allocator.** Orders are acquired from and released to a fixed-size pool rather than individually heap-allocated, avoiding allocation churn on the hot path.
- **`Order::level` back-pointer.** Each order points directly to the `PriceLevel` it's resting in, so cancellation doesn't need to look up or search for the right price level, it's a pointer read followed by an O(1) splice.
- **`std::flat_map` for price levels**, one for bids (ordered by `std::greater<>`, so best = highest price) and one for asks (best = lowest price by default ordering). This keeps both sides readable the same way (`begin()` is always the best price) and makes best-price lookup and iteration cache-friendly. Price levels are stored as `std::unique_ptr<PriceLevel>` rather than by value, since `flat_map`'s backing storage can reallocate on insert, which would invalidate the `Order::level` back-pointers if `PriceLevel` objects lived directly in the map.
- **`std::unordered_map` for order-ID lookup**, giving O(1) cancel-by-ID.
- **`std::expected` for error handling.** `AddOrder` returns `std::expected<AddOrderResult, AddOrderFailure>` rather than throwing, keeping the hot path free of exception-handling overhead. `AddOrderFailure` carries any trades that already executed before a failure (currently only pool exhaustion), so a partial match's trades are never silently dropped.

## Building

Requires a C++23 compiler and CMake 3.28+.

```bash
cmake -S . -B build
cmake --build build
```

## Testing

Tests use GoogleTest, fetched automatically via CMake's `FetchContent`.

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Project layout

```
lob-matching-engine/
├── include/lob/
│   ├── order.h           # Order struct, intrusive list pointers, PriceLevel back-pointer
│   ├── order_pool.h      # Free-list order allocator
│   ├── price_level.h     # FIFO queue of orders at one price
│   ├── order_book.h      # Both sides of the book, matching, cancel, best bid/ask
│   └── trade.h           # Trade, AddOrderResult, AddOrderFailure
├── src/                  # Implementations + main.cpp
└── tests/                # GoogleTest unit tests
```

## Status

Implemented: order resting, cancellation, and price-time priority matching (exact fills, partial fills, multi-level sweeps) for plain limit orders.

Not yet implemented: order types beyond plain limit (IOC, FOK, market), latency benchmarking, CI, and integration with a lock-free SPSC queue as the market-data ingestion path.
