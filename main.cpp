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

void testLoopPattern() {
    std::cout << "\n=== 测试场景2：循环扫描测试 ===" << std::endl;

    const int CAPACITY = 50;
    const int LOOP_SIZE = 500;
    const int OPERATIONS  = 200000;

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
        for (int key = 0; key < LOOP_SIZE / 5; ++key) {
            std::string value = "loop" + std::to_string(key);
            caches[i]->put(key, value);
        }

        int current_pos = 0;
        for (int op = 0; op < OPERATIONS; ++op) {
            bool isPut = (gen()%100 < 20);
            int key;
            if(op % 100 < 60){
                key = current_pos;
                current_pos = (current_pos + 1) % LOOP_SIZE;
            }else if (op % 100 < 90){
                key = gen() % LOOP_SIZE;
            }else{
                key = LOOP_SIZE + (gen() % LOOP_SIZE);
            }
            if(isPut){
                std::string value = "loop" + std::to_string(key) + "_v" + std::to_string(op % 100);
                caches[i]->put(key, value);
            }else{
                std::string result;
                get_operations[i]++;
                if(caches[i]->get(key, result))
                    hits[i]++;
            }
        }
    }
    printResults("循环扫描测试",CAPACITY,get_operations,hits);
}

void testWorkloadShift(){
    std::cout << "\n=== 测试场景3：工作负载剧烈变化测试 ===" << std::endl;
    const int CAPACITY = 30;
    const int OPERATIONS = 80000;
    const int PHASE_LENGTH = OPERATIONS / 5;

    FulinCache::FLruCache<int, std::string> lru(CAPACITY);
    FulinCache::FLfuCache<int, std::string> lfu(CAPACITY);
    FulinCache::ArcCache<int, std::string> arc(CAPACITY);

    std::random_device rd;
    std::mt19937 gen(rd());

    std::array<FulinCache::FICachePolicy<int, std::string>*, 3> caches = {&lru, &lfu,  &arc};
    std::vector<int> hits(3, 0);
    std::vector<int> get_operations(3, 0);
    std::vector<std::string> names={"LRU", "LFU", "ARC"};


    // 为每种缓存算法运行相同的测试
    for (int i = 0; i < caches.size(); ++i) {
        // 先预热缓存，只插入少量初始数据
        for (int key = 0; key < 30; ++key) {
            std::string value = "init" + std::to_string(key);
            caches[i]->put(key, value);
        }

        // 进行多阶段测试，每个阶段有不同的访问模式
        for (int op = 0; op < OPERATIONS; ++op) {
            // 确定当前阶段
            int phase = op / PHASE_LENGTH;

            // 每个阶段的读写比例不同 - 优化后的比例
            int putProbability;
            switch (phase) {
                case 0: putProbability = 15; break;  // 阶段1: 热点访问，15%写入更合理
                case 1: putProbability = 30; break;  // 阶段2: 大范围随机，降低写比例为30%
                case 2: putProbability = 10; break;  // 阶段3: 顺序扫描，10%写入保持不变
                case 3: putProbability = 25; break;  // 阶段4: 局部性随机，微调为25%
                case 4: putProbability = 20; break;  // 阶段5: 混合访问，调整为20%
                default: putProbability = 20;
            }

            // 确定是读还是写操作
            bool isPut = (gen() % 100 < putProbability);

            // 根据不同阶段选择不同的访问模式生成key - 优化后的访问范围
            int key;
            if (op < PHASE_LENGTH) {  // 阶段1: 热点访问 - 减少热点数量从10到5，使热点更集中
                key = gen() % 5;
            } else if (op < PHASE_LENGTH * 2) {  // 阶段2: 大范围随机 - 范围从1000减小到400，更适合20大小的缓存
                key = gen() % 400;
            } else if (op < PHASE_LENGTH * 3) {  // 阶段3: 顺序扫描 - 保持100个键
                key = (op - PHASE_LENGTH * 2) % 100;
            } else if (op < PHASE_LENGTH * 4) {  // 阶段4: 局部性随机 - 优化局部性区域大小
                // 产生5个局部区域，每个区域大小为15个键，与缓存大小20接近但略小
                int locality = (op / 800) % 5;  // 调整为5个局部区域
                key = locality * 15 + (gen() % 15);  // 每区域15个键
            } else {  // 阶段5: 混合访问 - 增加热点访问比例
                int r = gen() % 100;
                if (r < 40) {  // 40%概率访问热点（从30%增加）
                    key = gen() % 5;  // 5个热点键
                } else if (r < 70) {  // 30%概率访问中等范围
                    key = 5 + (gen() % 45);  // 缩小中等范围为50个键
                } else {  // 30%概率访问大范围（从40%减少）
                    key = 50 + (gen() % 350);  // 大范围也相应缩小
                }
            }

            if (isPut) {
                // 执行写操作
                std::string value = "value" + std::to_string(key) + "_p" + std::to_string(phase);
                caches[i]->put(key, value);
            } else {
                // 执行读操作并记录命中情况
                std::string result;
                get_operations[i]++;
                if (caches[i]->get(key, result)) {
                    hits[i]++;
                }
            }
        }
    }

    printResults("工作负载剧烈变化测试", CAPACITY, get_operations, hits);
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    testHotDataAccess();
    testLoopPattern();
    testWorkloadShift();
    return 0;
}
