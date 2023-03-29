cmake_minimum_required(VERSION 3.16)
project(llp_lab3 C)
set(CMAKE_C_COMPILER gcc)

find_package(PkgConfig REQUIRED)
pkg_search_module(THRIFT REQUIRED thrift_c_glib)
message(STATUS "THRIFT_INCLUDE_DIRS: ${THRIFT_INCLUDE_DIRS}")
message(STATUS "THRIFT_LIBRARIES: ${THRIFT_LIBRARIES}")

add_executable(
        client
        client/parser.tab.c
        client/parser.tab.h
        client/lex.yy.c
        client/graphql_ast.c
        client/graphql_ast.h
        client/client.c
        client/convertor.c
        gen-c_glib/structs_types.c
        gen-c_glib/structs_types.h
        gen-c_glib/zgdb_service.c
        gen-c_glib/zgdb_service.h
)

add_executable(
        server
        server/server.c
        client/graphql_ast.c
        client/graphql_ast.h
        gen-c_glib/structs_types.c
        gen-c_glib/structs_types.h
        gen-c_glib/zgdb_service.c
        gen-c_glib/zgdb_service.h
)

target_include_directories(client PUBLIC ${THRIFT_INCLUDE_DIRS})
target_include_directories(server PUBLIC ${THRIFT_INCLUDE_DIRS})
target_link_libraries(client PUBLIC ${THRIFT_LIBRARIES})
target_link_libraries(server PUBLIC ${THRIFT_LIBRARIES})
