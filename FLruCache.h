//
// Created by huoqi on 2025/8/5.
//

#ifndef FULINCACHE_FLRUCACHE_H
#define FULINCACHE_FLRUCACHE_H
#include<memory>
#include<unordered_map>
#include <mutex>
#include "FICachePolicy.h"

namespace FulinCache {
    template<typename Key, typename Value> class FLruCache;

    template <typename Key, typename Value>
    class LruNode{
    public:
        explicit LruNode(Key key, Value value):
        key_(key), value_(value),accessCount(1),next(nullptr), prev(){}

        Value getValue() const {return value_;}
        Key getKey() const {return key_;}
        void setValue(const Value& value) {value_ = value;}
        size_t getAccessCount() const {return accessCount;}
        void incrementAccessCount() {accessCount++;}

        friend class FLruCache<Key,Value>;
    private:
        size_t accessCount;
        Key key_;
        Value value_;
        std::shared_ptr<LruNode<Key,Value>> next;
        std::weak_ptr<LruNode<Key, Value>> prev;
    };

    template<typename Key, typename Value>
    class FLruCache: public FICachePolicy<Key, Value>{
    public:
        using LruNodeType = LruNode<Key,Value>;
        using NodePtr = std::shared_ptr<LruNodeType>;
        using NodeMap = std::unordered_map<Key,NodePtr>;

        explicit FLruCache(int capacity): capacity_(capacity){
            initializeCache();
        }

        ~FLruCache() override =default;

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

        void put(Key key, Value value) override{
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = nodeMap_.find(key);
            if(it != nodeMap_.end()){
                it->second->setValue(value);
                updateAccessCount(it->second);
                return;
            }
            if(capacity_ <= nodeMap_.size())
                removeLastNode();
            addNewNode(key, value);
        }

    private:
        void initializeCache(){
            head_ = std::make_shared<LruNodeType>(Key(),Value());
            tail_ = std::make_shared<LruNodeType>(Key(),Value());

            head_ -> next = tail_;
            tail_ -> prev = head_;
        }

        void addNewNode(const Key& key, const Value& value){
            NodePtr node = std::make_shared<LruNodeType>(key, value);
            nodeMap_[key] = node;
            addToFirst(node);
        }

        void updateAccessCount(NodePtr node){
            node->incrementAccessCount();
            moveToFront(node);
        }

        void moveToFront(NodePtr node){
            removeNode(node);
            addToFirst(node);
        }
        void addToFirst(NodePtr node){
            node->next = head_->next;
            node->prev = head_;
            head_->next->prev = node;
            head_->next = node;
        }

        void removeNode(NodePtr node){
            if(!node->prev.expired() && node->next){
                auto prev = node->prev.lock();
                prev->next = node->next;
                node->next->prev = node->prev;
                node->next = nullptr;
            }
        }
        void removeLastNode(){
            NodePtr node = tail_->prev.lock();
            if(node&& node!= head_){
                removeNode(node);
            }
        }

        NodePtr head_;
        NodePtr tail_;
        NodeMap nodeMap_;
        int capacity_;
        std::mutex mutex_;
    };

} // FulinCache

#endif //FULINCACHE_FLRUCACHE_H
