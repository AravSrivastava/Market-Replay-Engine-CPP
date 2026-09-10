#include "MatchingEngine.h"
#include "Order.h"
#include "orderbook.h"
#include <ios>

int main(){

    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    OrderBook orderbook;
    MatchingEngine matchingEngine;

    auto ask1 = std::make_unique<Order> (100, 1, 10, Side::Ask);
    auto ask2 = std::make_unique<Order> (100, 2, 15, Side::Ask);
    auto ask3 = std::make_unique<Order> (100, 3, 20, Side::Ask);
    auto ask4 = std::make_unique<Order> (100, 4, 10, Side::Ask);
    orderbook.addOrder(std::move(ask1));
    orderbook.addOrder(std::move(ask2));
    orderbook.addOrder(std::move(ask3));
    orderbook.addOrder(std::move(ask4));

    auto bid = std::make_unique<Order> (110,5,50,Side::Bid);

    matchingEngine.match(std::move(bid), orderbook);
    return 0;
}
