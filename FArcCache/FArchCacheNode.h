//
// Created by huoqi on 2025/8/6.
//

#ifndef FULINCACHE_FARCHCACHENODE_H
#define FULINCACHE_FARCHCACHENODE_H
#include<memory>

namespace FulinCache {
    template<typename Key,typename Value>
    class ArcLruPart;

    template<typename Key,typename Value>
    class ArcLfuPart;

    template<typename Key,typename Value>
    class FArchCacheNode {
    private:
        Key key_;
        Value value_;
        size_t accessCount;
        std::shared_ptr<FArchCacheNode<Key,Value>> next;
        std::weak_ptr<FArchCacheNode<Key,Value>> prev;

    public:
        FArchCacheNode()
        : key_(), value_(), accessCount(1), next(nullptr) {}
        FArchCacheNode(Key key, Value value)
        : key_(key), value_(value), accessCount(1), next(nullptr) {}

        Key getKey() const {return key_;}
        Value getValue() const {return value_;}
        void setValue(const Value& value) {value_ = value;}
        size_t getAccessCount() const {return accessCount;}
        void incrementAccessCount() {accessCount++;}

        friend class ArcLruPart<Key, Value>;

        friend class ArcLfuPart<Key, Value>;
    };

} // FulinCache

#endif //FULINCACHE_FARCHCACHENODE_H
