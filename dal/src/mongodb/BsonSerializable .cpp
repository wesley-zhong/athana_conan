#include "BsonSerializable .h"

#include <bsoncxx/builder/stream/document.hpp>
using namespace bsoncxx::builder::stream;

bsoncxx::document::value BsonSerializable::toBson() const {
    document doc{};
    return doc << finalize;
}

void  BsonSerializable::fromBson(bsoncxx::document::view v) {
};
