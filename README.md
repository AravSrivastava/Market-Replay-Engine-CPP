
# Market Replay Engine

A C++ project focused on building the core infrastructure behind an event-driven market replay and trading simulation system.

The project is currently centered around implementing a **price-time priority limit order book and matching engine**, with the long-term goal of building a complete market replay engine capable of processing historical market events through an order book and matching engine.

> **Current Status:** 🚧 Under active development

---

# Overview

Modern trading systems rely on efficient order books to maintain market state and process incoming orders. This project is an implementation-focused exploration of those underlying systems using modern C++.

The current implementation provides:

- Separate bid and ask books
- Price-time priority
- FIFO ordering within each price level
- Order lookup by order ID
- Order cancellation
- Best bid and best ask queries
- Explicit ownership management using `std::unique_ptr`
- Incoming order processing
- Price compatibility checks
- Partial order execution
- Full order execution
- Multi-level matching through a matching loop
- Removal of fully executed resting orders
- Resting of unfilled incoming orders

---

# Architecture

The current architecture consists of four primary components:

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
                 └────────┬─────────┘
                          │
                          ▼
                 ┌──────────────────┐
                 │ MatchingEngine   │
                 │──────────────────│
                 │ Price Crossing   │
                 │ Order Execution  │
                 │ Partial Fills    │
                 │ Full Fills       │
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
- Side
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
3. Order ownership
4. Order lookup

---

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
    std::map<
        Tprice,
        PriceLevel,
        std::greater<Tprice>
    >;
```

This allows the best bid to be accessed directly through:

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
using Asks =
    std::map<Tprice, PriceLevel>;
```

The best ask is therefore available through:

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

This ensures that there is a single clear owner for every resting order while price levels can efficiently maintain FIFO ordering.

---

# Matching Engine

The `MatchingEngine` is responsible for processing incoming orders against the existing order book.

The engine does not own the `OrderBook`. Instead, it operates on an existing order book:

```cpp
void match(
    std::unique_ptr<Order> mainOrder,
    OrderBook& orderbook
);
```

The incoming order is initially owned by the caller through a `std::unique_ptr`.

During matching, the engine accesses the order through a non-owning raw pointer:

```cpp
Order* order = mainOrder.get();
```

Ownership remains with `mainOrder` until the order is moved into the `OrderBook`.

---

## Matching Process

Conceptually, the matching process follows this flow:

```text
Incoming Order
      │
      ▼
Validate Order
      │
      ▼
Check Opposite Side
      │
      ▼
Find Best Compatible Price
      │
      ▼
Find Compatible Order
      │
      ▼
Execute Trade
      │
      ├───────────────┐
      ▼               ▼
Order Filled?     Resting Order Filled?
      │               │
      ▼               ▼
Stop Matching    Remove Resting Order
                      │
                      ▼
                 Continue Matching
```

---

# Price Compatibility

The matching engine checks whether an incoming order can execute against the best available price on the opposite side of the book.

### Incoming Ask

An ask can execute when:

```text
Incoming Ask Price ≤ Best Bid
```

For example:

```text
Incoming Ask: 100

Best Bid: 101

100 ≤ 101

MATCH
```

If:

```text
Incoming Ask: 103

Best Bid: 101

103 > 101

NO MATCH
```

---

### Incoming Bid

A bid can execute when:

```text
Incoming Bid Price ≥ Best Ask
```

For example:

```text
Incoming Bid: 105

Best Ask: 103

105 ≥ 103

MATCH
```

---

# Execution Quantity

The execution quantity is determined using the remaining quantities of both orders.

Conceptually:

```text
Execution Quantity =
min(
    Incoming Remaining Quantity,
    Resting Remaining Quantity
)
```

For example:

```text
Incoming Order: 100 shares
Resting Order:   40 shares

Execution:

40 shares
```

The resting order becomes filled while the incoming order retains:

```text
60 shares
```

The matching engine can then continue matching the remaining quantity against the next available compatible order.

---

# Partial and Full Fills

The current implementation handles the primary execution scenarios.

## Incoming Order Filled

```text
Incoming Order: Filled
Resting Order:  Active
```

Matching stops because the incoming order has no remaining quantity.

---

## Resting Order Filled

```text
Incoming Order: Active
Resting Order:  Filled
```

The filled resting order is removed from the order book.

Matching can then continue using the incoming order's remaining quantity.

---

## Both Orders Filled

```text
Incoming Order: Filled
Resting Order:  Filled
```

The resting order is removed from the order book and execution stops.

---

# Matching Loop

The matching engine uses a loop to continue execution while the incoming order still has remaining quantity:

```text
Incoming Order
      │
      ▼
Find Best Compatible Order
      │
      ▼
Execute
      │
      ├── Incoming Order Filled ──────► Stop
      │
      ├── No Compatible Order ────────► Stop
      │
      └── Resting Order Filled ───────► Continue
```

This allows a single incoming order to consume liquidity across multiple resting orders.

For example:

```text
Incoming Buy Order
Quantity: 100

Ask Book:

Price  Quantity
----------------
100      30
101      40
102      50
```

The matching engine can execute:

```text
100 @ 100
 30 @ 101
```

and then continue until the incoming order is fully executed or no compatible liquidity remains.

---

# Resting Unfilled Orders

If an incoming order cannot be completely executed, the remaining quantity can be placed into the order book.

Conceptually:

```text
Incoming Order
      │
      ▼
Matching Loop
      │
      ├── Fully Filled
      │        │
      │        ▼
      │      Done
      │
      └── Remaining Quantity
               │
               ▼
        Add to OrderBook
```

Ownership is transferred into the order book:

```cpp
orderbook.addOrder(std::move(mainOrder));
```

After this operation, the `OrderBook` becomes responsible for the lifetime of the resting order.

---

# Price-Time Priority

The order book follows the basic principle of price-time priority.

## Price Priority

For bids:

```text
Higher Price → Higher Priority
```

For asks:

```text
Lower Price → Higher Priority
```

---

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

# Supported Operations

## Order Book

- Add order
- Remove order
- Cancel order
- Lookup order by ID
- Retrieve best bid
- Retrieve best ask
- Maintain bid-side price levels
- Maintain ask-side price levels
- Preserve FIFO ordering

---

## Matching Engine

- Validate incoming orders
- Check whether an order already exists
- Check incoming order quantity
- Detect available liquidity on the opposite side
- Determine price compatibility
- Retrieve compatible resting orders
- Calculate execution quantity
- Execute partial fills
- Execute full fills
- Remove filled resting orders
- Continue matching while execution remains possible
- Rest remaining incoming quantity in the order book

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
├── matchingEngine.h
│   └── Order matching and execution logic
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

---

## Phase 2 — Matching Engine

- [x] Matching engine architecture
- [x] Marketable order detection
- [x] Price crossing logic
- [x] Partial fills
- [x] Full fills
- [x] Order quantity updates
- [x] Removal of completed resting orders
- [x] Multi-order matching loop
- [x] Resting unfilled incoming orders
- [ ] Trade generation
- [ ] Execution reporting
- [ ] Comprehensive testing

---

## Phase 3 — Market Replay

- [ ] Market event representation
- [ ] Historical data ingestion
- [ ] Event sequencing
- [ ] Replay loop
- [ ] Timestamp handling

---

## Phase 4 — System Improvements

- [ ] Unit testing
- [ ] Performance benchmarking
- [ ] Memory optimizations
- [ ] Profiling
- [ ] Concurrency exploration
- [ ] Atomic operations
- [ ] Market data adapters

---

# Design Goals

The project focuses on understanding and implementing the underlying systems concepts behind trading infrastructure.

Key areas of interest include:

- Modern C++
- Memory ownership
- Smart pointers
- Data structure design
- Price-time priority
- Order lifecycle management
- Matching algorithms
- Market microstructure
- Performance-oriented system design

---

# Current Focus

The core order book and the initial matching engine are now implemented.

The next stage of development is focused on:

1. **Testing the matching engine**
2. Verifying full and partial fill scenarios
3. Testing multi-order execution
4. Implementing trade generation
5. Improving error handling and edge-case coverage
6. Benchmarking the order book and matching engine

---

# Example System Flow

```text
Historical Market Events
          │
          ▼
┌───────────────────────┐
│    Market Replay      │
│                       │
│  Event Sequencing     │
│  Timestamp Handling   │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│    Matching Engine    │
│                       │
│  Price Compatibility  │
│  Order Execution      │
│  Partial Fills        │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│      Order Book       │
│                       │
│  Bids                 │
│  Asks                 │
│  Price Levels         │
└───────────────────────┘
            │
            ▼
        Trade Events
```

---

# Why This Project?

This project is being built primarily as a systems programming and market microstructure learning project.

Rather than relying entirely on existing trading frameworks, the goal is to understand how the underlying components work by implementing them from the ground up.

The project currently demonstrates concepts involving:

- Manual linked-list implementation
- STL associative containers
- Smart pointer ownership
- Raw non-owning pointers
- FIFO data structures
- Price-time priority
- Order matching
- Partial execution
- Full execution
- Order lifecycle management

The project will gradually evolve from a basic order book and matching engine into a broader market replay system.

---

## License

This project is currently under development. A license will be added as the project matures.
