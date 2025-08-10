//
// Created by huoqi on 2025/8/9.
//

#ifndef FULINCACHE_FLFUCACHE_H
#define FULINCACHE_FLFUCACHE_H
#include <memory>
#include <unordered_map>
#include <mutex>

#include "FICachePolicy.h"

namespace FulinCache{
    template<typename Key, typename Value> class FLfuCache;

    template<typename Key, typename Value>
    class FreqList{
    public:
        struct Node{
            Node(): accessCount(1), next(nullptr) {}
            Node(Key key, Value value):
                    key_(key), value_(value),accessCount(1),next(nullptr),prev(nullptr){}

            Value getValue() const {return value_;}
            Key getKey() const {return key_;}
            void setValue(const Value& value) {value_ = value;}
            size_t getAccessCount() const {return accessCount;}
            void incrementAccessCount() {accessCount++;}

            size_t accessCount;
            Key key_;
            Value value_;
            std::shared_ptr<Node> next;
            std::weak_ptr<Node> prev;
        };
        using NodePtr = std::shared_ptr<Node>;
        explicit FreqList(int n)
        : freq_(n){
            head_ = std::make_shared<Node>();
            tail_ = std::make_shared<Node>();
            head_->next = tail_;
            tail_->prev = head_;
        }

        bool empty() const{
            return head_->next == tail_;
        }

        void addToFront(NodePtr node){
            node->next = head_->next;
            node->prev = head_;
            head_->next ->prev =  node;
            head_->next = node;
        }

        void removeLast(){
            NodePtr last = tail_->prev.lock();
            if(last && last != head_){
                removeNode(last);
            }
        }
        void removeNode(NodePtr node){
            if(!node->prev.expired() && node->next){
                auto prev = node->prev.lock();
                prev->next = node->next;
                node->next->prev = node->prev;
                node->next = nullptr;
            }
        }


    private:
        NodePtr head_;
        NodePtr tail_;
        size_t freq_;
        friend class FLfuCache<Key,Value>;
};

    template<typename Key, typename Value>
    class FLfuCache: public FICachePolicy<Key, Value> {
    public:
        using NodeType = typename FreqList<Key,Value>::Node;
        using NodePtr = std::shared_ptr<NodeType>;
        using NodeMap = std::unordered_map<Key, NodePtr>;

        explicit FLfuCache(size_t capacity_, int maxAverageAccess = 100000)
        : capacity_(capacity_)
        , minFreq_(0)
        , maxAverageAccess_(maxAverageAccess)
        , totalAccessCount_(0)
        , currentAverageAccess_(0)
        {}

        void put(Key key, Value value) override{
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = nodeMap_.find(key);
            if(it != nodeMap_.end()){
                it->second->setValue(value);
                updateAccessCount(it->second);
                return;
            }
            if(nodeMap_.size() >= capacity_)
                evictLeastFrequent();
            addNewNode(key, value);
        }

        bool get(Key key, Value& value) override{
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = nodeMap_.find(key);
            if(it != nodeMap_.end()){
                value = it->second->getValue();
                updateAccessCount(it->second);
                return true;
            }
            return false;
        }

        Value get(Key key) override{
            Value value{};
            get(key, value);
            return value;
        }


    private:
        void evictLeastFrequent(){
            auto it = freqMap_.find(minFreq_);
            if(it == freqMap_.end())
                return;
            freqMap_[minFreq_]->removeLast();
            totalAccessCount_ -= minFreq_;
            if(freqMap_[minFreq_]->empty()){
                freqMap_.erase(minFreq_);
                minFreq_ = 1; // Put
            }
        }
        void addNewNode(const Key& key, const Value& value){
            NodePtr node = std::make_shared<NodeType>(key, value);
            nodeMap_[key] = node;
            if(freqMap_.find(1) == freqMap_.end())
                freqMap_[1] = new FreqList<Key,Value>();
            freqMap_[1] ->addToFront(node);
        }
        void updateAccessCount(NodePtr node){
            size_t oldFreq = node->getAccessCount();
            freqMap_[oldFreq] ->removeNode(node);
            node->incrementAccessCount();
            totalAccessCount_++;
            size_t newFreq = node->getAccessCount();
            if(minFreq_ == oldFreq && freqMap_[minFreq_]->empty()){
                minFreq_++;
            }
            if(freqMap_.find(newFreq) == freqMap_.end())
                freqMap_[newFreq] = new FreqList<Key, Value>();
            freqMap_[newFreq] -> addToFront(node);
        }

        size_t capacity_;
        NodeMap nodeMap_;
        std::unordered_map<size_t, std::unique_ptr<FreqList<Key,Value>>> freqMap_;
        size_t minFreq_;

        size_t totalAccessCount_;
        size_t currentAverageAccess_;
        size_t maxAverageAccess_;

        std::mutex mutex_;
    };
}

#endif //FULINCACHE_FLFUCACHE_H
