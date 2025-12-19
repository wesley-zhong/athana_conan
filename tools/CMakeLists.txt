cmake_minimum_required(VERSION 3.16)
project(tools VERSION 0.1.0 LANGUAGES C CXX)

message("jjjjjjjjjjjjjjjjjjjjjjjjjjjjj")
add_custom_command(
        COMMAND python3 ./bson_codegen/codegen_bson.py "${PROJECT_SOURCE_DIR}../game/src/dos" "${PROJECT_SOURCE_DIR}../game/src/code-gen"
)