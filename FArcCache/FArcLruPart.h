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

        void put(Key key, Value value){
            if(capacity_<=0) return;
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = mainCache_.find(key);
            if(it != mainCache_.end()){
                updateExistingNode(it->second, value);
                return;
            }
            if(mainCache_.size() >= capacity_)
                evictLeastRecent();
            addNewNode(key, value);
        }

        bool get(Key key, Value& value, bool& shouldTransform){
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = mainCache_.find(key);
            if(it != mainCache_.end()){
                shouldTransform = updateAccessCount(it->second);
                value = it->second->getValue();
                return true;
            }
            return false;
        }

        bool checkGhost(Key key){
            auto it = ghostCache_.find(key);
            if(it != ghostCache_.end()){
                ghostCache_.erase(it);
                removeFromGhost(it->second);
                return true;
            }
            return false;
        }

        void increaseCapacity(){
            capacity_++;
        }

        bool decreaseCapacity(){
            if(capacity_ <= 0) return false;
            if(mainCache_.size() >= capacity_)
                evictLeastRecent();
            capacity_--;
            return true;
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

        void addNewNode(const Key& key, const Value& value){
            NodePtr node = std::make_shared<NodeType>(key, value);
            mainCache_[key] = node;
            addToFront(node);
        }

        bool  updateAccessCount(NodePtr node){
            node->incrementAccessCount();
            moveToFront(node);
            return transformThreshold_ <= node->getAccessCount();
        }

        void updateExistingNode(NodePtr node, const Value& value){
            node->setValue(value);
            updateAccessCount(node);
        }

        void moveToFront(NodePtr node){
            removeFromMain(node);
            addToFront(node);
        }

        void addToFront(NodePtr node){
            node->next = head_->next;
            node->prev = head_;
            head_->next->prev = node;
            head_->next = node;
        }

        void removeFromGhost(NodePtr node){
            if(!node->prev.expired() && node->next){
                auto prev = node->prev.lock();
                prev->next = node->next;
                node->next->prev = node->prev;
                node->next = nullptr;
            }
        }

        void removeFromMain(NodePtr node){
            if(!node->prev.expired() && node->next){
                auto prev = node->prev;
                node->next = prev->next;
                node->prev = node->next->prev;
                node->next = nullptr;
            }
        }

        void removeLastGhost(){
            if(!tail_->prev.expired()){
                NodePtr lastNode = ghostTail_->prev.lock();
                if(lastNode == ghostHead_) return;
                ghostCache_.erase(lastNode->getKey());
                removeFromGhost(lastNode);
            }
        }

        void addToGhost(NodePtr node){
            node->accessCount = 1;  // 重置计数

            node->next  = ghostHead_->next;
            node->prev = ghostHead_;
            ghostHead_->next->prev = node;
            ghostHead_->next = node;

            ghostCache_[node->getKey()] = node;
        }

        void evictLeastRecent(){
            auto leastRecent = tail_->prev.lock();
            if(leastRecent && leastRecent!= head_){
                removeFromMain(leastRecent);
                mainCache_.erase(leastRecent->getKey());
                if(ghostCache_.size() >= ghostCapacity_){
                    removeLastGhost();
                }
                addToGhost(leastRecent);
            }
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
