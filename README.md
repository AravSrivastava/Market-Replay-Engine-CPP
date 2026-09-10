# Market Replay Engine

A performance-oriented C++ project for building the core infrastructure of an event-driven market replay and trading simulation system.

The project is being built from the ground up to explore **market microstructure, matching engines, order books, memory ownership, data structures, and performance-oriented systems programming**.

> **Current Status:** 🚧 v1 Matching Engine Complete — Market Replay Infrastructure In Progress

---

## Overview

The current implementation provides a **price-time priority limit order book** and matching engine capable of processing incoming orders, matching them against resting liquidity, executing partial and full fills, and maintaining the state of the book.

The long-term goal is to evolve this into a complete market replay engine capable of processing historical market events through the order book and matching engine.

### Current capabilities

- Price-time priority matching
- Separate bid and ask books
- FIFO ordering within each price level
- Best bid / best ask queries
- Order lookup by order ID
- Order cancellation
- Partial order execution
- Full order execution
- Multi-level order matching
- Resting unfilled orders
- Explicit ownership through `std::unique_ptr`
- Non-owning order pointers within price levels
- C++23 build configuration
- CMake + Ninja build system
- Debug, Release, and RelWithDebInfo build presets
- Dedicated performance benchmark

---

## Architecture

The current engine is composed of four primary components:

```text
                    ┌─────────────────────┐
                    │       Order         │
                    │─────────────────────│
                    │ Order ID            │
                    │ Price               │
                    │ Quantity            │
                    │ Remaining Quantity  │
                    │ Side                │
                    │ Status              │
                    └──────────┬──────────┘
                               │
                               ▼
                    ┌─────────────────────┐
                    │     PriceLevel      │
                    │─────────────────────│
                    │ Price               │
                    │ FIFO Order Queue    │
                    │ Head / Tail         │
                    └──────────┬──────────┘
                               │
                               ▼
                    ┌─────────────────────┐
                    │     OrderBook       │
                    │─────────────────────│
                    │ Bid Price Levels    │
                    │ Ask Price Levels    │
                    │ Order Lookup        │
                    └──────────┬──────────┘
                               │
                               ▼
                    ┌─────────────────────┐
                    │   MatchingEngine    │
                    │─────────────────────│
                    │ Price Compatibility │
                    │ Order Matching      │
                    │ Trade Execution     │
                    │ Partial / Full Fill │
                    └─────────────────────┘
```

### Order

`Order` represents an individual order entering or resting in the book.

An order contains:

- Order ID
- Price
- Initial quantity
- Remaining quantity
- Side
- Status
- Links to neighboring orders within a price level

Prices and quantities use integer-based types to avoid floating-point precision issues.

```cpp
using Tprice = std::int64_t;
using TorderID = std::uint64_t;
using Tquantity = std::uint64_t;
```

---

### PriceLevel

A `PriceLevel` manages all orders resting at the same price.

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

This preserves FIFO ordering within a price level.

Supported operations include:

- Add order to the tail
- Remove the head order
- Remove the tail order
- Remove an arbitrary order
- Track the number of orders at the level

---

### OrderBook

The `OrderBook` maintains three primary structures:

```text
OrderBook
│
├── bids_
│      └── price → PriceLevel
│
├── asks_
│      └── price → PriceLevel
│
└── orderLookUp_
       └── order ID → unique_ptr<Order>
```

Bids are stored in descending price order:

```cpp
using Bids =
    std::map<Tprice, PriceLevel, std::greater<Tprice>>;
```

Asks are stored in ascending price order:

```cpp
using Asks =
    std::map<Tprice, PriceLevel>;
```

This allows the best bid and best ask to be obtained from the beginning of their respective maps.

Orders are owned by the order lookup structure:

```cpp
using OrderLookUp =
    std::unordered_map<TorderID, std::unique_ptr<Order>>;
```

Price levels maintain non-owning `Order*` pointers.

This separates:

- Order ownership
- Price-level ordering
- Order lookup

while maintaining a single owner for each resting order.

---

## Matching Engine

`MatchingEngine` processes incoming orders against the existing order book.

The matching process is conceptually:

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
Find Compatible Resting Order
      │
      ▼
Execute Trade
      │
      ├───────────────┐
      ▼               ▼
Incoming Filled?   Resting Filled?
      │               │
      ▼               ▼
   Stop         Remove Resting Order
                      │
                      ▼
                Continue Matching
```

### Price compatibility

For an incoming ask:

```text
Incoming Ask Price <= Best Bid
```

For an incoming bid:

```text
Incoming Bid Price >= Best Ask
```

### Execution quantity

Each execution uses:

```text
min(
    incoming remaining quantity,
    resting remaining quantity
)
```

This supports:

- Partial fills
- Full fills
- Multiple executions against different price levels

If the incoming order still has quantity after matching, the remaining quantity is added to the order book.

---

## Price-Time Priority

The order book follows standard price-time priority.

### Price priority

For bids:

```text
Higher Price → Higher Priority
```

For asks:

```text
Lower Price → Higher Priority
```

### Time priority

Orders at the same price are processed FIFO:

```text
First Order In
      ↓
Highest Priority

Second Order
      ↓
Lower Priority

Third Order
      ↓
Lowest Priority
```

The `PriceLevel` linked list maintains this ordering.

---

## Project Structure

The repository is organized around the separation between core engine code, correctness tests, and performance benchmarks.

```text
Market-Replay-Engine-CPP/
│
├── include/
│   ├── MatchingEngine.h
│   ├── Order.h
│   ├── orderbook.h
│   └── priceLevel.h
│
├── tests/
│   └── test.cpp
│
├── benchmarks/
│   └── testing.cpp
│
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── .gitignore
└── .vscode/
```

### Directory responsibilities

| Directory | Purpose |
|---|---|
| `include/` | Core order book and matching-engine implementation |
| `tests/` | Functional/correctness tests |
| `benchmarks/` | Performance workloads |
| `.vscode/` | VS Code project configuration |

---

# Building

The project uses:

- **C++23**
- **CMake**
- **Ninja**
- **Clang**

CMake Presets provide separate build configurations for development, optimized execution, and profiling.

### Configure and build Debug

```bash
cmake --preset debug
cmake --build --preset debug
```

The resulting executables are located under:

```text
build/debug/
```

### Configure and build Release

```bash
cmake --preset release
cmake --build --preset release
```

Release builds are intended for performance measurements.

### Configure and build RelWithDebInfo

```bash
cmake --preset relwithdebinfo
cmake --build --preset relwithdebinfo
```

This configuration is intended for profiling optimized code while retaining debugging information.

---

# Running

## Functional test

After building the Debug configuration:

```bash
./build/debug/market_replay_test
```

The functional test exercises the order book and matching engine with a small set of orders.

## Performance benchmark

Build the Release configuration:

```bash
cmake --preset release
cmake --build --preset release
```

Then run:

```bash
./build/release/market_replay_benchmark
```

The benchmark currently processes a large synthetic workload through the matching engine and reports:

- Total execution time
- Orders per second
- Total trades executed
- Best bid
- Best ask

The benchmark is intended to provide a repeatable baseline for future performance work.

---

# Performance

The current benchmark processes:

```text
20,000,000 orders
```

A recent benchmark run achieved approximately:

```text
~998,000 orders/second
```

The benchmark is not intended to represent production exchange performance. Its purpose is to establish a repeatable baseline for evaluating future changes to:

- Data structures
- Memory allocation
- Matching logic
- Order lookup
- Logging
- Cache behavior
- Compiler optimizations
- Other performance-sensitive components

Future performance measurements will be compared against this baseline.

---

# Design Goals

This project is primarily a **systems programming and market microstructure learning project**.

The implementation focuses on understanding the underlying mechanisms rather than relying on an existing trading framework.

Key areas of interest include:

- Modern C++
- C++23
- Data structure design
- Price-time priority
- Order lifecycle management
- Matching algorithms
- Memory ownership
- Smart pointers
- Non-owning pointers
- STL associative containers
- FIFO data structures
- Market microstructure
- Performance-oriented design
- Profiling and optimization

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

## Phase 2 — Matching Engine v1

- [x] Matching engine architecture
- [x] Marketable order detection
- [x] Price crossing logic
- [x] Partial fills
- [x] Full fills
- [x] Order quantity updates
- [x] Removal of completed resting orders
- [x] Multi-order matching loop
- [x] Resting unfilled incoming orders
- [x] Basic execution handling
- [x] Initial performance benchmark

## Phase 3 — Market Replay

- [ ] Market event representation
- [ ] Historical market-data ingestion
- [ ] Event sequencing
- [ ] Replay loop
- [ ] Timestamp handling
- [ ] Market-data adapters

## Phase 4 — Testing & Performance

- [ ] Comprehensive unit tests
- [ ] Edge-case testing
- [ ] Sanitizer builds
- [ ] Profiling
- [ ] Memory optimization
- [ ] Allocation analysis
- [ ] Cache behavior analysis
- [ ] Benchmark suite
- [ ] Logging performance analysis

## Phase 5 — Systems Extensions

- [ ] Concurrency exploration
- [ ] Atomic operations where appropriate
- [ ] Additional market-data infrastructure
- [ ] Replay performance improvements

---

# Development Philosophy

The engine is intentionally being developed incrementally.

The objective is not simply to produce a fast matching engine, but to understand the engineering decisions behind one:

```text
Data Structures
      ↓
Memory Ownership
      ↓
Order Book
      ↓
Matching Engine
      ↓
Testing
      ↓
Benchmarking
      ↓
Profiling
      ↓
Optimization
      ↓
Market Replay
```

Performance improvements will be measured against reproducible benchmarks rather than assumed from code changes.

---

# Current Focus

The first version of the core matching engine is complete.

The immediate development focus is now shifting toward:

1. Expanding correctness testing
2. Improving edge-case coverage
3. Building a stronger benchmark suite
4. Profiling the current implementation
5. Investigating memory and allocation behavior
6. Improving the logging path
7. Beginning the market replay layer

The project will gradually evolve from a standalone order book and matching engine into a broader historical market replay system.

---

## License

This project is currently under development and does not yet have a finalized open-source license.
