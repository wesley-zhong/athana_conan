//
// Created by zhongweiqi on 2026/1/13.
//

#ifndef ATHENA_ATHENACONFIG_H
#define ATHENA_ATHENACONFIG_H
#include <iostream>
#include "toml++/toml.hpp"
#include "core/log/XLog.h"
#include <string>
#include <string_view>
class AthenaConfig {
public:
    // 获取全局单例
    static AthenaConfig &instance() {
        static AthenaConfig inst;
        return inst;
    }

    // 加载配置文件
    bool load(const std::string &filename) {
        try {
            table_ = toml::parse_file(filename);
            return true;
        } catch (const toml::parse_error &err) {
            ERR_LOG("Error parsing file {}, erro={}", filename, err.description());
            return false;
        }
    }

    // 获取任意类型的值（带默认值方案）
    template<typename T>
    T get(std::string_view section, std::string_view key, T default_val) const {
        // 使用 toml++ 的链式访问
        return table_[section][key].value_or(default_val);
    }

    // 获取字符串（专门优化）
    std::string getString(std::string_view section, std::string_view key, std::string_view default_val = "") const {
        return table_[section][key].value_or(std::string(default_val));
    }

    // 检查某个 section 是否存在
    bool hasSection(std::string_view section) const {
        return table_.contains(section) && table_[section].is_table();
    }

    /**
 * @brief 获取数组类型并转换为 std::vector
 * @tparam T 元素类型，如 int64_t, std::string, double
 */
    template<typename T>
    std::vector<T> getArray(std::string_view section, std::string_view key) const {
        std::vector<T> result;

        // 1. 获取数组节点
        if (auto arr = table_[section][key].as_array()) {
            result.reserve(arr->size());

            // 2. 遍历并尝试转换每个元素
            for (auto&& item : *arr) {
                // value<T>() 如果类型不匹配或为空会返回空 optional
                if (auto val = item.value<T>()) {
                    result.push_back(*val);
                }
            }
        } else {
            WARN_LOG("Config key {}.{} is not an array or not found", section, key);
        }

        return result;
    }

private:
    AthenaConfig() = default;

    toml::table table_;
};

#endif //ATHENA_ATHENACONFIG_H
