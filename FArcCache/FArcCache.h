//
// Created by huoqi on 2025/8/9.
//

#ifndef FULINCACHE_FARCCACHE_H
#define FULINCACHE_FARCCACHE_H
#include<memory>

#include "FArcLruPart.h"
#include "FArcLfuPart.h"
#include "../FICachePolicy.h"


namespace FulinCache{
    template<typename Key, typename Value>
    class ArcCache: public FICachePolicy<Key, Value>{
    public:
        ArcCache(size_t capacity, size_t transformThreshold)
        : capacity_(capacity)
        , transformThreshold_(transformThreshold)
        , lruPart_(new ArcLruPart<Key, Value>(capacity,transformThreshold))
        , lfuPart_(new ArcLfuPart<Key, Value>(capacity)){}

        ~ArcCache() = default;

        bool get(Key key, Value& value) override{
            checkGhostCaches(key);
            bool shouldTransform = false;
            if(lruPart_->get(key, value, shouldTransform)){
                if(shouldTransform)
                    lfuPart_->put(key, value);
            }
            return lfuPart_->get(key, value);
        }

        Value get(Key key) override{
            Value value{};
            get(key, value);
            return value;
        }

        void put(Key key, Value value) override{
            checkGhostCaches(key);
            bool inLfu = lfuPart_->contains(key);
            lruPart_->put(key,value);
            if(inLfu)
                lfuPart_->put(key,value);
        }
    private:
        bool checkGhostCaches(Key key){
            bool inGhost = false;
            if(lruPart_->checkGhost(key)) {
                if (lfuPart_->decreaseCapacity())
                    lruPart_->increaseCapacity();
                inGhost = true;
            }
            else if(lfuPart_->checkGhost(key)) {
                if (lruPart_->decreaseCapacity())
                    lfuPart_->increaseCapacity();
                inGhost = true;
            }
            return inGhost;
        }

        size_t transformThreshold_;
        size_t capacity_;
        std::unique_ptr<ArcLfuPart<Key, Value>> lfuPart_;
        std::unique_ptr<ArcLruPart<Key, Value>> lruPart_;
    };
}

#endif //FULINCACHE_FARCCACHE_H
