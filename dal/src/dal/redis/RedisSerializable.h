#pragma once

#include <string>
#include <string_view>

namespace dal
{
    class RedisSerializable
    {
    public:
        virtual ~RedisSerializable() = default;

        virtual std::string toString() const = 0;

        virtual void fromString(std::string_view sv) = 0;
    };
} // namespace dal
