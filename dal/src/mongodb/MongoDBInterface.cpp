//
// Created by Wesly Zhong on 2025/11/23.
//
#include "core/log/XLog.h"
#include "MongoDBInterface.h"

int MongoDBInterface::testInit() {
    mongocxx::instance instance;
    // Replace the placeholder with your Atlas connection string
    mongocxx::uri uri("mongodb://admin:admin@localhost:27017/?authSource=admin");
    // Create a mongocxx::client with a mongocxx::options::client object to set the Stable API version
    // mongocxx::options::client client_options;
    mongocxx::client client(uri);
    // Ping the server to verify that the connection works
    auto admin = client["admin"];
    auto command = make_document(kvp("ping", 1));
    auto result = admin.run_command(command.view());
    INFO_LOG(bsoncxx::to_json(result));
    INFO_LOG("Pinged your deployment. You successfully connected to MongoDB!");

    auto gameDB = client["sf_fashi_game"];
    auto roleCollection = gameDB["playerworld"];
    auto find_one_result  = roleCollection.find_one(make_document(kvp("_id", 100000707)));
    if (find_one_result ) {
        INFO_LOG("uuuuu value = {}", bsoncxx::to_json(find_one_result.value()));
    }
    return 0;
}
