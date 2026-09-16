//
// Created by zhongweiqi on 2026/9/16.
//

#ifndef ATHENA_SNOWFLAKE_H
#define ATHENA_SNOWFLAKE_H

#include <mutex>
#include <thread>
#include "core/common/XTime.h"
#include "core/common/XAssert.h"

// Twitter Snowflake 雪花算法
//
// 64 bit ID 布局:
//   1 bit  符号位, 恒为 0
//  41 bit  毫秒时间戳, 自定义纪元起, 可用约 69.7 年
//  10 bit  机器 ID, 0 ~ 1023, 多机部署时由部署方分配保证不重复
//  12 bit  同毫秒内序列号, 单机单毫秒最多 4096 个
//
// 生成的 ID 按时间趋势递增; 线程安全
class Snowflake {
public:
    explicit Snowflake(int64 workerId)
        : workerId_(workerId), lastTimestamp_(0), sequence_(0)
    {
        XAssert(workerId >= 0 && workerId <= MAX_WORKER_ID);
    }

    // 生成一个 ID
    // 时钟回拨时自旋等待时钟追上上次的生成时间
    int64 nextId()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        int64 timestamp = XTime::currentTimeMillis();
        if (timestamp < lastTimestamp_) {
            do {
                std::this_thread::yield();
                timestamp = XTime::currentTimeMillis();
            } while (timestamp < lastTimestamp_);
        }
        if (timestamp == lastTimestamp_) {
            sequence_ = (sequence_ + 1) & SEQUENCE_MASK;
            if (sequence_ == 0) {
                // 当前毫秒序列号用尽, 自旋到下一毫秒
                do {
                    std::this_thread::yield();
                    timestamp = XTime::currentTimeMillis();
                } while (timestamp <= lastTimestamp_);
            }
        } else {
            sequence_ = 0;
        }
        lastTimestamp_ = timestamp;
        return ((timestamp - EPOCH) << TIMESTAMP_SHIFT)
               | (workerId_ << WORKER_ID_SHIFT)
               | sequence_;
    }

    // 从 ID 还原生成时的毫秒时间戳 (Unix 纪元)
    static int64 getTimestamp(int64 id)
    {
        return (id >> TIMESTAMP_SHIFT) + EPOCH;
    }

private:
    // 位分配
    static const int64 WORKER_ID_BITS = 10;
    static const int64 SEQUENCE_BITS = 12;
    static const int64 MAX_WORKER_ID = (1LL << WORKER_ID_BITS) - 1;
    static const int64 SEQUENCE_MASK = (1LL << SEQUENCE_BITS) - 1;
    static const int64 WORKER_ID_SHIFT = SEQUENCE_BITS;
    static const int64 TIMESTAMP_SHIFT = SEQUENCE_BITS + WORKER_ID_BITS;
    // 自定义纪元: 2020-01-01 00:00:00 UTC
    static const int64 EPOCH = 1577836800000LL;

    int64 workerId_;
    int64 lastTimestamp_;
    int64 sequence_;
    std::mutex mutex_;
};

#endif //ATHENA_SNOWFLAKE_H
