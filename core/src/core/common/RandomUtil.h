//
// Created by zhongweiqi on 2026/2/9.
//

#ifndef ATHENA_RANDOMUTIL_H
#define ATHENA_RANDOMUTIL_H

#include "BaseType.h"
#include <random>
#include <vector>
#include <type_traits>
#include <algorithm>

class RandomUtil {
public:
    static int32 getInt(int32 min, int32 max) {
        if (min > max) std::swap(min, max);

        // 使用 thread_local 确保每个线程有独立的引擎，既线程安全又避开了锁的开销
        std::uniform_int_distribution<int32> dist(min, max);
        return dist(getEngine());
    }

    // 随机洗牌容器
    static void shuffle(std::vector<int32> &container) {
        std::shuffle(container.begin(), container.end(), getEngine());
    }


private:
    // 获取当前线程的随机数引擎
    static std::mt19937 &getEngine() {
        // 使用 random_device 获取硬件种子
        // thread_local 保证了该变量在每个线程生命周期内只初始化一次
        static thread_local std::mt19937 engine(std::random_device{}());
        return engine;
    }
};


#endif //ATHENA_RANDOMUTIL_H
