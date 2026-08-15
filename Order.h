#pragma once
#include <iostream>
#include <cstdint>

enum class Side{
    Ask,
    Bid
};

enum class Status{
    Inactive,
    Active,
    Filled
};

using Tprice = std::int64_t;
using TorderID= std::uint64_t;
using Tquantity = std:: uint64_t;

class Order{
    //Public entity of the order class
    Tquantity remainingQuantity_;

    public:
        //Contructor for order class
        Order(Tprice price, TorderID orderID, Tquantity quantity, Side side):

        price_{price},
        orderID_{orderID},
        initialQuantity_{quantity},
        remainingQuantity_{quantity},
        side_{side},
        status_{Status::Active}

        {}

        //Public API's to call.
        Tprice getPrice() const {return price_;}
        Tquantity getRemainingQuantity() const {return remainingQuantity_; }
        TorderID getOrderID() const {return orderID_;}
        Side getSide() const {return side_;}
        Status getStatus() const {return status_;}

        //Function
        void PrintOrder(){
            std:: cout <<
            "Order ID: " <<orderID_ <<'\n' <<
            "Initial Quantity: " << initialQuantity_<<'\n' <<
            "Remaining Quantity: " << remainingQuantity_<<'\n' <<
            "Price: " << price_  <<'\n' <<
            "Side: " << static_cast<int>(side_) <<'\n';
        }
        //Methods for Order class
        void cancelOrder() { status_ = Status::Inactive ;}

            private:
        //Private entities of the order class
        const Tprice price_;
        const TorderID orderID_;
        const Tquantity initialQuantity_;
        const Side side_;
        Status status_;
};
