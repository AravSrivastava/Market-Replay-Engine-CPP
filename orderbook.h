/*
 * Orderbook owns 3 things all price level in a map by price.
 * OrderLookUp map
 *
 */
#pragma once
#include "Order.h"
#include "priceLevel.h"
#include <algorithm>
#include <memory>
#include <print>
#include <unordered_map>
#include <map>
#include <optional>

using Bids = std:: map<Tprice, PriceLevel, std::greater<Tprice> >;
using Asks = std:: map<Tprice, PriceLevel>;
using OrderLookUp = std:: unordered_map<TorderID, std::unique_ptr<Order>>;

class OrderBook{
    private:
        Bids bids_;
        Asks asks_;
        OrderLookUp orderLookUp_;
    public:

        bool OrderExists(TorderID id) const{
            return orderLookUp_.contains(id);
        }

        bool askIsEmpty() const{
                return asks_.empty();
        }

        bool bidIsEmpty() const {
            return bids_.empty();
        }

        bool addOrder(std:: unique_ptr<Order> o){
            auto id = o -> getOrderID();
            if(orderLookUp_.contains(id)){
                std:: print("[ addOrder ] Order is already locaed");
                return false;
            }
            Side side = o -> getSide();
            auto price = o -> getPrice();
            //Need to add the order in the priceLevel Queue and in the bids / asks and in the orderlookup
            // 1. Add in the priceLevel and asks_/bids_
            Bids::iterator bidCheck;
            Asks::iterator askCheck;
            if(side == Side::Ask){
                askCheck = asks_.find(price);
                if(askCheck == asks_.end() ){
                    auto result = asks_.insert({price, PriceLevel(price)});
                    askCheck = result.first;
                }
                askCheck->second.addOrder(o.get());
            }
            else if(side == Side::Bid){
                bidCheck = bids_.find(price);
                if(bidCheck == bids_.end()){
                    auto result = bids_.insert({price, PriceLevel(price)});
                    bidCheck = result.first;
                }
                bidCheck -> second.addOrder(o.get());
            }
            //Adding the OrderId and the unique_ptr in the orderLookUp structure
            orderLookUp_.insert({id, std::move(o)});
            return true;
        }

        bool removeOrder(TorderID id){
            //To cancel an order in the orderbook we would need to remove it from the priceLevel queue, then the asks_ or bids_ and after that remove the pointer from the orderLookUp to end its scope.

            //Check if the order exists or not

            if(!OrderExists(id)){
                std::print("[cancelOrder]The Order you are looking for does not exist");
                return false;

            }
            //Now we know that the order exists so now we will be removing the order from the priceLevel
            // we need to get the orderId

            auto order = orderLookUp_.find(id)->second.get();
            Side side = order->getSide();
            Tprice price = order -> getPrice();
            //To remove the order from the priceLevel queue.

            if(side == Side::Ask){
                auto it = asks_.find(price);
                it->second.removeOrder(order);
                if(it ->second.getCount() == 0){
                    asks_.erase(it);
                }
            }
            else{
                auto it = bids_.find(price);
                it ->second.removeOrder(order);
                if(it ->second.getCount() == 0){
                    bids_.erase(it);
                }
            }

            //To remove the order from the orderLookUp map that contains the ownership of the order ptr
            orderLookUp_.erase(id);

            return true;
        }

        std:: optional<Tprice> getBestAsk() const {
            if(asks_.empty()){return std::nullopt;}
            return asks_.begin()-> first ;
        }
        std:: optional<Tprice> getBestBid() const {
            if(bids_.empty()){
                return std::nullopt;
            }
            return bids_.begin()-> first ;
        }

        Order* getBidPriceLevel(Tprice price) const {
            auto it = bids_.find(price);
            return it->second.returnHead();
        }
        Order* getAskPriceLevel(Tprice price) const {
            auto it = asks_.find(price);
            return it -> second.returnHead();
        }

};


//Unordered map will store the orderId as its key
//Need to add getBidPriveLevel(Tprice price) -> Helper function to get the priceLevel head.
//Need to add getAskPriveLevel(Tprice price) -> Helper function to get the priveLevel head.
