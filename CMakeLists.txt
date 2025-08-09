cmake_minimum_required(VERSION 3.28)
project(FulinCache)

set(CMAKE_CXX_STANDARD 17)

add_executable(FulinCache main.cpp
        FLruCache.h
        FICachePolicy.h
        FICachePolicy.h
        FArcCache/FArchCacheNode.h
        FArcCache/FArchCacheNode.h
        FArcCache/FArcLruPart.h
        FArcCache/FArcLfuPart.h
        FArcCache/FArcLfuPart.h
        FArcCache/FArcCache.h
        FArcCache/FArcCache.h
)
