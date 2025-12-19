#include "RoleDO.hpp"
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/types.hpp>

bsoncxx::document::value RoleDO::toBson() const {
    bsoncxx::builder::stream::document doc{};
    {
        auto __view = BsonSerializable::toBson().view();
        for (auto&& e : __view) {
            doc << e.key() << e.get_value();
        }
    }
    doc << "_id" << _id;
    doc << "name" << name;
    return doc << bsoncxx::builder::stream::finalize;
}

void RoleDO::fromBson(bsoncxx::document::view v) {
    BsonSerializable::fromBson(v);
    if (auto e = v["_id"])
        _id = e.get_int64();
    if (auto e = v["name"])
        name = e.get_utf8().value.to_string();
}
