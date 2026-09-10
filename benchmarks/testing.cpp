#include <iostream>
#include <memory>
#include <chrono>

#include "orderbook.h"
#include "MatchingEngine.h"

std::unique_ptr<Order> createOrder(
    TorderID id,
    Side side,
    Tprice price,
    Tquantity quantity
) {
    return std::make_unique<Order>(price, id, quantity, side);
}

int main() {
    constexpr int NUM_ORDERS = 20000000;

    OrderBook orderbook;
    MatchingEngine engine;

    std::cout << "=== Market Replay Engine Performance Test ===\n";
    std::cout << "Sending " << NUM_ORDERS
              << " orders to the matching engine...\n";

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_ORDERS; ++i) {

        Side side = (i % 2 == 0)
                  ? Side::Bid
                  : Side::Ask;

        Tprice price = 100 + (i % 50);
        Tquantity quantity = 10 + (i % 90);

        auto order = createOrder(
            i + 1,
            side,
            price,
            quantity
        );

        // Send the order through the matching engine.
        engine.match(std::move(order), orderbook);
    }

    auto end = std::chrono::high_resolution_clock::now();

    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start
        );

    double seconds = duration.count() / 1000.0;

    double ordersPerSecond =
        NUM_ORDERS / seconds;

    std::cout << "\nTest completed in "
              << duration.count()
              << " ms\n";

    std::cout << "Throughput: "
              << ordersPerSecond
              << " orders/second\n"
              <<"Total Trades Exexuted: "
              << engine .get_totalTrades() <<'\n';

    if (auto bestBid = orderbook.getBestBid()) {
        std::cout << "Best Bid: " << bestBid.value() << '\n';
    }

    if (auto bestAsk = orderbook.getBestAsk()) {
        std::cout << "Best Ask: " << bestAsk.value() << '\n';
    }

    return 0;
}
