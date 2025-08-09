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
        key_(key), value_(value),accessCount(0),next(nullptr),prev(nullptr){}

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
        std::shared_ptr<LruNode> next;
        std::weak_ptr<LruNode> prev;
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

        bool get(Key key, Value& value) override{

        }

        Value get(Key key) override{

        }

        void put(Key key, Value value) override{

        }

    private:
        void initializeCache(){
            head_ = std::make_shared<LruNodeType>(Key(),Value());
            tail_ = std::make_shared<LruNodeType>(Key(),Value());

            head_ -> next = tail_;
            tail_ -> prev = head_;
        }

        NodePtr head_;
        NodePtr tail_;
        NodeMap nodeMap_;
        int capacity_;
        std::mutex mutex_;
    };

} // FulinCache

#endif //FULINCACHE_FLRUCACHE_H
