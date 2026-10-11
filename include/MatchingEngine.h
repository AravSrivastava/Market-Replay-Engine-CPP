#pragma once
#include "Order.h"
#include "orderbook.h"
#include <cassert>
#include <cstdint>
#include <optional>
//#include "priceLevel.h"
//#include <print>
//#include <type_traits>

enum class Executable{
    MayContinue,
    StopExecution
};

    class MatchingEngine{
        private:
            uint64_t totalTrades_ = 0;
        public:

            //Helper functions for matching.
            bool orderExists(Order* order, const OrderBook& orderbook){
                return orderbook.OrderExists(order -> getOrderID());
            }
            bool quantityCheck(Order* order){
                return (order ->getRemainingQuantity() > 0);
            }
            bool checkIncomingMatch(Order* order, const OrderBook& orderbook){
                return priceCompatibility(order, orderbook).has_value();
            }

            std::optional<Tprice> priceCompatibility(Order* order, const OrderBook& orderbook){
                auto bestPrice = order -> getPrice();
                if(order -> getSide() ==  Side::Ask){
                    auto bestBid = orderbook.getBestBid();
                    if( bestBid != std::nullopt){
                        if( bestPrice> bestBid.value() ){
                            return std::nullopt;
                        }else{
                            return bestBid;
                        }
                    }
                }else{
                    auto bestAsk = orderbook.getBestAsk();
                    if(bestAsk != std::nullopt){
                        if(order -> getPrice() < bestAsk.value()){
                            return std::nullopt;
                        }else{
                            return bestAsk;
                        }
                    }
                }
                return std::nullopt;
            }
            Order* getCompatibleOrder(std::optional<Tprice> price,Order* order, const OrderBook& orderbook){
                //Getting the side of the order
                if(price == std::nullopt){
                    return nullptr;
                }
                Side side = order -> getSide();
                if(side == Side::Ask){
                    return orderbook.getBidPriceLevel(price.value());
                }else{
                    return orderbook.getAskPriceLevel(price.value());
                }
                return nullptr;
            }
            Tquantity getExecutionQuantity(Order* incomingOrder, Order* compatibleOrder){
                auto incomingOrderRemainingQuantity = incomingOrder -> getRemainingQuantity();
                auto compatibleOrderRemainingQuantity =  compatibleOrder -> getRemainingQuantity();
                if( incomingOrderRemainingQuantity < compatibleOrderRemainingQuantity){
                    return incomingOrderRemainingQuantity;
                }
                else if (compatibleOrderRemainingQuantity < incomingOrderRemainingQuantity){
                    return compatibleOrderRemainingQuantity;
                }
                else{
                    return incomingOrderRemainingQuantity;
                }
            }
            //The match function
            Executable execute(Order* incoming, OrderBook& book) {
    auto bestPrice = priceCompatibility(incoming, book);
    if (!bestPrice) return Executable::StopExecution;

    Order* resting = getCompatibleOrder(bestPrice, incoming, book);
    if (!resting) return Executable::StopExecution;

    Tquantity execQty = getExecutionQuantity(incoming, resting);
    if (execQty == 0) return Executable::StopExecution;

    incoming->updateQuantity(execQty);
    resting->updateQuantity(execQty);
    totalTrades_++;

    bool incomingFilled = (incoming->getRemainingQuantity() == 0);
    bool restingFilled = (resting->getRemainingQuantity() == 0);

    if (incomingFilled) incoming->orderFilled();
    if (restingFilled) resting->orderFilled();

    if (restingFilled) {
        book.removeOrder(resting->getOrderID());
    }

    if (incomingFilled) {
        return Executable::StopExecution;  // Incoming done, don't re-add
    }
    return Executable::MayContinue;  // Incoming has more qty, continue matching
}



        public:
            uint64_t get_totalTrades() const { return totalTrades_; }

            void match(std::unique_ptr<Order> mainOrder, OrderBook& orderbook){
                if(not mainOrder) return;
                Order* order = mainOrder.get();
                if(order == nullptr){
                    return;
                }
                //This checks if the order exists in the orderbook or not
                //std::print("[MatchingEngine: match] The order already exists");
                if(orderExists(order,orderbook)){
                    return;
                }
                if(not quantityCheck(order)){
                    //std::print("[MatchingEngine: match] The order has no quantity");
                    return;
                }
                if(not checkIncomingMatch(order, orderbook)){
                    orderbook.addOrder(std::move(mainOrder));
                    return;
                }
                while(order -> getRemainingQuantity() > 0){
                    if(execute(order, orderbook) ==  Executable::StopExecution){
                        //std::println("[Matching Engine: Match] Execution is stopped");
                        break;
                    }
                }
                if(order -> getRemainingQuantity() > 0 ){
                    //std::println("[Matching Engine: Match] Adding order to the orderbook");
                    orderbook.addOrder(std::move(mainOrder));
                }
            }
    };
