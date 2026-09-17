#include "QuestDO.hpp"
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/types.hpp>

bsoncxx::document::value QuestDO::toBson() const {
    bsoncxx::builder::stream::document doc{};
    {
        auto __view = dal::BsonSerializable::toBson().view();
        for (auto&& e : __view) {
            doc << e.key() << e.get_value();
        }
    }
    doc << "_id" << _id;
 //   doc << "questIdsFinished" << questIdsFinished;
   // doc << "curQuestIds" << curQuestIds;
    return doc << bsoncxx::builder::stream::finalize;
}

void QuestDO::fromBson(bsoncxx::document::view v) {
    dal::BsonSerializable::fromBson(v);
    if (auto e = v["_id"])
        _id = e.get_int64();
    // unsupported field type: std::vector<int64_t> questIdsFinished
    // unsupported field type: std::vector<int64_t> curQuestIds
}
