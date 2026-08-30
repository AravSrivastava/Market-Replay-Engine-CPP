    #pragma once
    #include "Order.h"

    //This is the class which is resposible for managing the elements in the Order Queue
    //We are implementing a queue strucutre for each price and this priceLevel is the structure that will be managing that queue
    class PriceLevel{
        private:
        Tprice price_;
        Order* head_ = nullptr;
        Order* tail_ = nullptr;
        std:: size_t count_;
        public:

        //Public API's
        std:: size_t getCount() const {return count_;}
        Tprice getPrice() const {return price_;}

        PriceLevel(Tprice price):
            price_(price),
            count_(0)
            {}

        void addOrder(Order* order) {
            if(head_ == nullptr){
                order->prev_ = nullptr;
                order -> next_ = nullptr;
                head_ = order;
                tail_ = order;
                count_++;
                return ;
            }
            order->prev_ = tail_;
            order->next_ = nullptr;
            order -> prev_ -> next_ = order;
            tail_ = order;
            count_++;
            return;
        }
        bool removeOrderHead() {
            if (head_ == nullptr) {
                return false;
            }

            Order* temp = head_;
            head_ = head_->next_;

            if (head_ != nullptr) {
                head_->prev_ = nullptr;
            } else {
                tail_ = nullptr;
            }

            temp->next_ = nullptr;
            temp->prev_ = nullptr;
            count_--;
            return true;
        }

        bool removeOrderTail(){
            if(tail_ == nullptr ){
                return false;
            }
            Order* temp = tail_;
            tail_ = tail_ -> prev_;
            if(tail_ == nullptr){
                head_ = nullptr;
            }else{
                tail_ -> next_ = nullptr;
            }

            temp -> next_ = nullptr;
            temp -> prev_ = nullptr;

            count_--;
            return true;
        }

        bool removeOrder(Order *order){
            if(order-> next_ == nullptr){
                removeOrderTail();
                return true;
            }
            if(order -> prev_ == nullptr){
                removeOrderHead();
                return true;
            }
            Order *temp = order;
            order -> prev_ -> next_ = temp -> next_;
            order -> next_ -> prev_ = temp -> prev_;

            temp -> prev_ = nullptr;
            temp -> next_ = nullptr;
            count_--;
            return true;
        }
    };
