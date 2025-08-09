//
// Created by huoqi on 2025/8/9.
//

#ifndef FULINCACHE_FARCLFUPART_H
#define FULINCACHE_FARCLFUPART_H
#include<memory>
#include <map>
#include <list>
#include <unordered_map>
#include <mutex>
#include "FArchCacheNode.h"
namespace FulinCache{
    template<typename Key, typename Value>
    class ArcLfuPart{
    public:
        using NodeType = FArchCacheNode<Key, Value>;
        using NodePtr = std::shared_ptr<NodeType>;
        using NodeMap = std::unordered_map<Key, NodePtr>;
        using FreqMap = std::map<size_t, std::list<NodePtr>>;

        explicit ArcLfuPart(size_t capacity)
        : capacity_(capacity)
        , ghostCapacity_(capacity)
        , minFreq_(0){
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
            if(capacity_ <= mainCache_.size())
                evictLeastFrequent();
            addNewNode(key, value);
        }

        bool get(Key key, Value& value){
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = mainCache_.find(key);
            if(it != mainCache_.end()){
                updateAccessCount(it->second);
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
            if(capacity_<=0) return false;
            if(capacity_<=mainCache_.size())
                evictLeastFrequent();
            capacity_--;
            return true;
        }

    private:

        void initializeLists(){
            ghostHead_ = std::make_shared<NodeType>();
            ghostTail_ = std::make_shared<NodeType>();

            ghostHead_->next = ghostTail_;
            ghostTail_->prev = ghostHead_;
        }

        void addNewNode(const Key& key, const Value& value){
            NodePtr node = std::make_shared<NodeType>(key, value);
            mainCache_[key] = node;
            minFreq_ = 1;
            if(freqMap_.find(minFreq_) == freqMap_.end())
                freqMap_[minFreq_] = std::list<NodePtr>();
            freqMap_[minFreq_].push_front(node);
        }

        void updateExistingNode(NodePtr node, Value value){
            node->setValue(value);
            updateAccessCount(node);
        }

        void updateAccessCount(NodePtr node){
            size_t oldFreq = node->getAccessCount();
            node->incrementAccessCount();
            size_t newFreq = node->getAccessCount();

            auto& minFreqList = freqMap_[oldFreq];
            minFreqList.erase(oldFreq);
            if(minFreqList.empty()){
                freqMap_.erase(oldFreq);
                if(minFreq_ == oldFreq)
                    minFreq_ = newFreq;
            }
            if(freqMap_.find(newFreq) == freqMap_.end())
                freqMap_[newFreq] = std::list<NodePtr>();
            freqMap_[newFreq].push_front(node);
        }

        void removeFromGhost(NodePtr node){
            if(!node->prev.lock() && node->next){
                auto prev = node->prev;
                prev->next = node->next;
                node->next ->prev = node->prev;
                node->next = nullptr;
            }
        }

        void removeLastGhost(){
            NodePtr lastGhost = ghostTail_->prev.lock();
            if(lastGhost && lastGhost != ghostHead_){
                removeFromGhost(lastGhost);
                ghostCache_.erase(lastGhost->getKey());
            }
        }

        void addToGhost(NodePtr node){
            node->next = ghostHead_->next;
            node->prev = ghostHead_;
            ghostHead_->next->prev = node;
            ghostHead_->next=node;

            ghostCache_[node->getKey()] = node;
        }

        void evictLeastFrequent(){
            if(freqMap_.empty()) return;
            auto& minFreqList = freqMap_[minFreq_];
            if(minFreqList.empty()) return;

            NodePtr leastNode = minFreqList.back();
            minFreqList.pop_back();
            if(minFreqList.empty()){
                freqMap_.erase(minFreq_);
                if(!freqMap_.empty()){
                    minFreq_ = freqMap_.begin()->first;
                }
            }

            if(ghostCapacity_ <= ghostCache_.size())
                removeLastGhost();
            addToGhost(leastNode);
        }

        NodePtr ghostHead_;
        NodePtr ghostTail_;
        FreqMap freqMap_;

        NodeMap mainCache_;
        NodeMap ghostCache_;

        size_t capacity_;
        size_t ghostCapacity_;
        size_t minFreq_;

        std::mutex mutex_;
    };
}

#endif //FULINCACHE_FARCLFUPART_H
