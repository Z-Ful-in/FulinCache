//
// Created by huoqi on 2025/8/6.
//

#ifndef FULINCACHE_FARCLRUPART_H
#define FULINCACHE_FARCLRUPART_H

#include <unordered_map>
#include <mutex>
#include "FArchCacheNode.h"
namespace FulinCache{
    template<typename Key, typename Value>
    class ArcLruPart{
    public:
        using NodeType = FArchCacheNode<Key, Value>;
        using NodePtr = std::shared_ptr<NodeType>;
        using NodeMap = std::unordered_map<Key, NodePtr>;

        explicit ArcLruPart(size_t capacity, size_t transformThreshold)
        : capacity_(capacity)
        , ghostCapacity_(capacity)
        , transformThreshold_(transformThreshold){
            initializeLists();
        }



    private:
        void initializeLists(){
            head_ = std::make_shared<NodeType>(Key(), Value());
            tail_ = std::make_shared<NodeType>(Key(), Value());
            ghostHead_  = std::make_shared<NodeType>(Key(), Value());
            ghostTail_ = std::make_shared<NodeType>(Key(), Value());

            head_->next = tail_;
            tail_->prev=  head_;

            ghostHead_->next = ghostTail_;
            ghostTail_->prev= ghostHead_;
        }

        NodePtr head_;
        NodePtr tail_;

        NodePtr ghostHead_;
        NodePtr ghostTail_;

        NodeMap mainCache_;
        NodeMap ghostCache_;

        size_t capacity_;
        size_t ghostCapacity_;
        std::mutex mutex_;
        size_t transformThreshold_;
    };
}

#endif //FULINCACHE_FARCLRUPART_H
