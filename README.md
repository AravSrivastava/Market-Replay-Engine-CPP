# Market Replay Engine

A C++ project focused on building the core infrastructure behind an event-driven market replay and trading simulation system.

The project is currently centered around implementing a **price-time priority limit order book**, with the long-term goal of building a complete market replay engine capable of processing historical market events through an order book and matching engine.

> **Current Status:** 🚧 Under active development

---

## Overview

Modern trading systems rely on efficient order books to maintain market state and process incoming orders. This project is an implementation-focused exploration of those underlying systems using modern C++.

The current implementation provides the foundation for a limit order book with:

- Separate bid and ask books
- Price-time priority
- FIFO ordering within each price level
- Order lookup by order ID
- Order cancellation
- Best bid and best ask queries
- Explicit ownership management using `std::unique_ptr`

The next major component will be the **Matching Engine**, responsible for processing executable orders against the existing order book.

---

## Architecture

The current architecture consists of three primary components:

```text
                 ┌──────────────────┐
                 │      Order       │
                 │──────────────────│
                 │ Order ID         │
                 │ Price            │
                 │ Quantity         │
                 │ Side             │
                 │ Status           │
                 └────────┬─────────┘
                          │
                          ▼
                 ┌──────────────────┐
                 │   PriceLevel     │
                 │──────────────────│
                 │ Price            │
                 │ FIFO Order Queue │
                 │ Head / Tail      │
                 └────────┬─────────┘
                          │
                          ▼
                 ┌──────────────────┐
                 │    OrderBook     │
                 │──────────────────│
                 │ Bid Levels       │
                 │ Ask Levels       │
                 │ Order Lookup     │
                 └──────────────────┘
```

---

# Core Components

## Order

The `Order` class represents an individual order in the market.

Each order contains:

- Order ID
- Price
- Initial quantity
- Remaining quantity
- Side (`Bid` or `Ask`)
- Status
- Links to neighboring orders within a price level

### Type Definitions

```cpp
using Tprice = std::int64_t;
using TorderID = std::uint64_t;
using Tquantity = std::uint64_t;
```

Using integer-based price types avoids floating-point precision issues when representing prices.

### Order Side

```cpp
enum class Side {
    Ask,
    Bid
};
```

### Order Status

```cpp
enum class Status {
    Inactive,
    Active,
    Filled
};
```

---

# PriceLevel

A `PriceLevel` represents all orders resting at the same price.

Orders at the same price are maintained using a manually implemented doubly linked list:

```text
HEAD
  │
  ▼
[Order] ⇄ [Order] ⇄ [Order]
                        │
                        ▼
                       TAIL
```

This structure preserves **FIFO ordering**, which is required for price-time priority.

### Supported Operations

- Add an order to the tail
- Remove the head order
- Remove the tail order
- Remove an arbitrary order
- Track the number of orders at the price level

The linked-list structure allows orders to maintain their position within a price level without storing the queue separately.

---

# OrderBook

The `OrderBook` manages:

1. Bid-side price levels
2. Ask-side price levels
3. Order ownership and lookup

## Bid Book

Bids are stored in descending price order:

```text
Highest Bid
    │
    ▼
[105] → [104] → [103]
```

```cpp
using Bids =
    std::map<Tprice,
             PriceLevel,
             std::greater<Tprice>>;
```

This allows the best bid to be accessed directly from:

```cpp
bids_.begin()
```

---

## Ask Book

Asks are stored in ascending price order:

```text
Lowest Ask
    │
    ▼
[106] → [107] → [108]
```

```cpp
using Asks = std::map<Tprice, PriceLevel>;
```

The best ask is therefore available directly from:

```cpp
asks_.begin()
```

---

# Order Ownership

Orders are owned by the order lookup structure:

```cpp
using OrderLookUp =
    std::unordered_map<
        TorderID,
        std::unique_ptr<Order>
    >;
```

This separates:

- **Ownership** of orders
- **Price-level ordering**
- **Order lookup**

The order lookup owns each `Order` using `std::unique_ptr`.

Price levels only store non-owning raw pointers:

```text
OrderBook
│
├── orderLookUp_
│      │
│      └── unique_ptr<Order>
│
├── bids_
│      │
│      └── PriceLevel
│              │
│              └── Order*
│
└── asks_
       │
       └── PriceLevel
               │
               └── Order*
```

This ensures that there is a single clear owner for every order while price levels can efficiently maintain FIFO ordering.

---

# Supported Operations

## Add Order

Orders can be added to the order book:

```cpp
bool addOrder(std::unique_ptr<Order> order);
```

The order is:

1. Checked for duplicate order IDs
2. Inserted into the appropriate price level
3. Linked into the FIFO queue
4. Stored in the order lookup map

---

## Cancel Order

Orders can be cancelled using their order ID:

```cpp
bool cancelOrder(TorderID id);
```

Cancellation involves:

1. Looking up the order
2. Finding its price level
3. Removing it from the linked list
4. Removing an empty price level
5. Removing the owning `unique_ptr` from the lookup map

---

## Best Bid

```cpp
std::optional<Tprice> getBestBid() const;
```

Returns:

```text
Highest active bid price
```

or:

```cpp
std::nullopt
```

when no bids are available.

---

## Best Ask

```cpp
std::optional<Tprice> getBestAsk() const;
```

Returns:

```text
Lowest active ask price
```

or:

```cpp
std::nullopt
```

when no asks are available.

---

# Price-Time Priority

The order book follows the basic principle of price-time priority.

## Price Priority

For bids:

```text
Higher price → Higher priority
```

For asks:

```text
Lower price → Higher priority
```

## Time Priority

Orders at the same price are processed in FIFO order:

```text
First Order In
      ↓
Highest Priority

Second Order
      ↓

Third Order
      ↓
Lowest Priority
```

The `PriceLevel` linked list maintains this ordering.

---

# Current Project Structure

```text
Market-Replay-Engine-C-
│
├── Order.h
│   └── Order representation and core types
│
├── priceLevel.h
│   └── FIFO queue management for individual prices
│
├── orderbook.h
│   └── Bid/ask books and order ownership
│
└── README.md
```

---

# Development Roadmap

## Phase 1 — Order Book Foundation

- [x] Order representation
- [x] Bid and ask sides
- [x] Price levels
- [x] FIFO ordering within price levels
- [x] Order lookup by ID
- [x] Order cancellation
- [x] Best bid query
- [x] Best ask query

## Phase 2 — Matching Engine

- [ ] Matching engine architecture
- [ ] Marketable order detection
- [ ] Price crossing logic
- [ ] Partial fills
- [ ] Full fills
- [ ] Order quantity updates
- [ ] Removal of completed orders
- [ ] Trade generation

## Phase 3 — Market Replay

- [ ] Market event representation
- [ ] Historical data ingestion
- [ ] Event sequencing
- [ ] Replay loop
- [ ] Timestamp handling

## Phase 4 — System Improvements

- [ ] Unit testing
- [ ] Performance benchmarking
- [ ] Memory optimizations
- [ ] Concurrency exploration
- [ ] Atomic operations
- [ ] Market data adapters

---

# Design Goals

The project focuses on understanding and implementing the underlying systems concepts behind trading infrastructure.

Key areas of interest include:

- Modern C++
- Memory ownership
- Data structure design
- Price-time priority
- Order lifecycle management
- Matching algorithms
- Market microstructure
- Performance-oriented system design

---

# Current Focus

The current focus is designing and implementing the **Matching Engine**.

The `MatchingEngine` will operate on an `OrderBook` rather than owning it directly, allowing the engine to work with different order books while keeping ownership responsibilities clearly separated.

Conceptually:

```text
Incoming Order
      │
      ▼
┌─────────────────┐
│ Matching Engine │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   Order Book    │
│                 │
│  Bids    Asks   │
└─────────────────┘
         │
         ▼
      Trades
```

The matching engine will become the core component responsible for determining whether incoming orders can execute against resting liquidity.

---

# Why This Project?

This project is being built primarily as a systems programming and market microstructure learning project.

Rather than relying entirely on existing trading frameworks, the goal is to understand how the underlying components work by implementing them from the ground up.

The project will gradually evolve from a basic order book into a broader market replay system.

---

## License

This project is currently under development. A license will be added as the project matures.
