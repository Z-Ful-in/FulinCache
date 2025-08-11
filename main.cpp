#include <iostream>
#include <random>
#include <iomanip>
#include "FLfuCache.h"
#include "FLruCache.h"
#include "FArcCache/FArcCache.h"
#include <windows.h>
#include <io.h>


void printResults(const std::string& testName, int capacity,
                  const std::vector<int>& get_operations,
                  const std::vector<int>& hits){
    std::cout <<"===" << testName <<"结果汇总==="<<std::endl;
    std::cout<<"缓存大小:" << capacity << std::endl;

    std::vector<std::string> names;
    if(hits.size() == 3){
        names = {"LRU", "LFU", "ARC"};
    }else if(hits.size() == 4){
        names = {"LRU", "LFU", "ARC", "LRU-K"};
    }else if(hits.size() == 5){
        names = {"LRU", "LFU", "ARC", "LRU-K", "LFU-Aging"};
    }

    for(size_t i =0; i < hits.size(); ++i){
        double hitRate = 100.0 * hits[i] / get_operations[i];
        std::cout << (i < names.size()? names[i] : "Algorithm" + std::to_string(i + 1))
        << " - 命中率:" << std::fixed << std::setprecision(2)
        << hitRate << "% ";
        std::cout << "(" << hits[i] << "/" << get_operations[i] << ")" << std::endl;
    }
    std::cout << std::endl;
}

void testHotDataAccess(){
    std::cout << "\n=== 测试场景1： 热点数据访问测试 ===" << std::endl;
    const int CAPACITY = 20;
    const int OPERATIONS = 500000;
    const int HOT_KEYS = 20;
    const int COLD_KEYS = 5000;

    FulinCache::FLruCache<int, std::string> lru(CAPACITY);
    FulinCache::FLfuCache<int, std::string> lfu(CAPACITY);
    FulinCache::ArcCache<int, std::string> arc(CAPACITY);

    std::random_device rd;
    std::mt19937 gen(rd());

    std::array<FulinCache::FICachePolicy<int, std::string>*, 3> caches = {&lru, &lfu,  &arc};
    std::vector<int> hits(3, 0);
    std::vector<int> get_operations(3, 0);
    std::vector<std::string> names={"LRU", "LFU", "ARC"};

    for (int i = 0; i < caches.size(); ++i) {
        for (int key = 0; key < HOT_KEYS; ++key) {
            std::string value = "value" + std::to_string(key);
            caches[i] -> put(key, value);
        }
        for (int op = 0; op < OPERATIONS; ++op) {
            bool isPut = (gen()%100 < 30);
            int key;

            if(gen()%100 < 70){
                key = gen() % HOT_KEYS; // hot data
            }else{
                key = HOT_KEYS + (gen() % COLD_KEYS); // cold data
            }

            if(isPut){
                std::string value = "value" + std::to_string(key) + "_v" + std::to_string(op % 100);
                caches[i] -> put(key, value);
            }else{
                std::string result;
                get_operations[i]++;
                if(caches[i] ->get(key, result)){
                    hits[i] ++;
                }
            }
        }
    }
    printResults("工作负载剧烈变化测试", CAPACITY, get_operations, hits);
}


int main() {
    SetConsoleOutputCP(CP_UTF8);
    testHotDataAccess();
    return 0;
}
