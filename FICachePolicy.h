//
// Created by huoqi on 2025/8/5.
//

#ifndef FULINCACHE_FICACHEPOLICY_H
#define FULINCACHE_FICACHEPOLICY_H

namespace FulinCache {
    template<typename Key, typename Value>
    class FICachePolicy {
    public:
        FICachePolicy() = default;
        ~FICachePolicy() = delete;

        virtual void put(Key key, Value value) = 0;

        virtual bool get(Key key, Value& value) = 0;

        virtual Value get(Key key) = 0;
    };

} // FulinCache

#endif //FULINCACHE_FICACHEPOLICY_H
