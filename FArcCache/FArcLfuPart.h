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
