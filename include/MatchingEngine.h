#pragma once
#include "Order.h"
#include "priceLevel.h"
#include "orderbook.h"
#include <cassert>
#include <cstdint>
#include <optional>
#include <print>
#include <type_traits>

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
                Side side = order -> getSide();
                if(side ==  Side::Ask){
                    if(orderbook.bidIsEmpty()){
                        return false;
                    }
                }else{
                    if(orderbook.askIsEmpty()){
                        return false;
                    }
                }
                return true;
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
            Executable execute(Order* order, OrderBook& orderbook){
                if(order == nullptr){
                    return Executable::StopExecution;
                }
                auto bestPrice = priceCompatibility(order,orderbook);
                Order* compatibleOrder = getCompatibleOrder(bestPrice, order,  orderbook);
                if (compatibleOrder == nullptr){
                    return Executable::StopExecution;
                }
                Tquantity executionQuantity = getExecutionQuantity(order, compatibleOrder);
                if(executionQuantity <= 0){
                    return Executable::StopExecution;
                }
                //Execution Process
                order -> updateQuantity(executionQuantity);
                if(order -> getRemainingQuantity() == 0){
                    order -> orderFilled();
                }
                compatibleOrder -> updateQuantity(executionQuantity);
                totalTrades_++;
                if(compatibleOrder -> getRemainingQuantity() == 0){
                    compatibleOrder -> orderFilled();
                }

                if(order -> getStatus() == Status::Filled && compatibleOrder ->getStatus() == Status ::Active){
                    return Executable::StopExecution;
                }
                else if(compatibleOrder -> getStatus() == Status::Filled && order -> getStatus() == Status::Active){
                    orderbook.removeOrder(compatibleOrder -> getOrderID());
                    return Executable::MayContinue;
                }
                else if(order->getStatus() == Status::Filled && compatibleOrder->getStatus() == Status::Filled) {
                    if(orderbook.OrderExists(order -> getOrderID())){
                        orderbook.removeOrder(order -> getOrderID());
                    }
                    orderbook.removeOrder(compatibleOrder-> getOrderID());

                    return Executable:: StopExecution;
                }
                else{
                    return Executable::StopExecution;
                }
            }



        public:
            uint64_t get_totalTrades() const { return totalTrades_; }

            void match(std::unique_ptr<Order> mainOrder, OrderBook& orderbook){
                Order* order = mainOrder.get();
                if(order == nullptr){
                    return;
                }
                //This checks if the order exists in the orderbook or not
                if(orderExists(order,orderbook)){
                    //std::print("[MatchingEngine: match] The order already exists");
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
