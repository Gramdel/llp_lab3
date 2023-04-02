cmake_minimum_required(VERSION 3.16)
project(llp_lab3 C)
set(CMAKE_C_COMPILER gcc)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(THRIFT REQUIRED thrift_c_glib)

set(CLIENT_DIR client)
set(SERVER_DIR server)
set(DB_DIR db)
set(GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gen-c_glib)

flex_target(LEXER "${CLIENT_DIR}/lexer.l" "${CMAKE_CURRENT_SOURCE_DIR}/${CLIENT_DIR}/lex.yy.c")
bison_target(PARSER "${CLIENT_DIR}/parser.y" "${CMAKE_CURRENT_SOURCE_DIR}/${CLIENT_DIR}/parser.tab.c")
add_flex_bison_dependency(LEXER PARSER)

add_custom_target(gen_thrift thrift --gen c_glib structs.thrift
        BYPRODUCTS
        ${GEN_DIR}/structs_types.c
        ${GEN_DIR}/structs_types.h
        ${GEN_DIR}/zgdb_service.c
        ${GEN_DIR}/zgdb_service.h
        WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
        )

add_executable(client_exec
        ${CLIENT_DIR}/parser.tab.c
        ${CLIENT_DIR}/parser.tab.h
        ${CLIENT_DIR}/lex.yy.c
        ${CLIENT_DIR}/graphql_ast.c
        ${CLIENT_DIR}/graphql_ast.h
        ${CLIENT_DIR}/serializer.c
        ${CLIENT_DIR}/serializer.h
        ${CLIENT_DIR}/client.c
        ${GEN_DIR}/structs_types.c
        ${GEN_DIR}/structs_types.h
        ${GEN_DIR}/zgdb_service.c
        ${GEN_DIR}/zgdb_service.h
        )

add_executable(server_exec
        ${DB_DIR}/utils/optional.h
        ${DB_DIR}/utils/optional.c
        ${DB_DIR}/zgdb/format.c
        ${DB_DIR}/zgdb/format.h
        ${DB_DIR}/zgdb/document.h
        ${DB_DIR}/zgdb/document.c
        ${DB_DIR}/zgdb/list.c
        ${DB_DIR}/zgdb/list.h
        ${DB_DIR}/zgdb/document_public.h
        ${DB_DIR}/zgdb/format_public.h
        ${DB_DIR}/zgdb/element.c
        ${DB_DIR}/zgdb/element.h
        ${DB_DIR}/zgdb/element_public.h
        ${DB_DIR}/zgdb/query.h
        ${DB_DIR}/zgdb/query.c
        ${DB_DIR}/zgdb/query_public.h
        ${DB_DIR}/zgdb/iterator.c
        ${DB_DIR}/zgdb/iterator.h
        ${DB_DIR}/zgdb/iterator_public.h
        ${DB_DIR}/zgdb/schema.c
        ${DB_DIR}/zgdb/schema.h
        ${DB_DIR}/zgdb/schema_public.h
        ${DB_DIR}/zgdb/condition.c
        ${DB_DIR}/zgdb/condition.h
        ${DB_DIR}/zgdb/condition_public.h
        ${SERVER_DIR}/server.c
        ${GEN_DIR}/structs_types.c
        ${GEN_DIR}/structs_types.h
        ${GEN_DIR}/zgdb_service.c
        ${GEN_DIR}/zgdb_service.h
        server/deserializer.c server/deserializer.h)

target_include_directories(client_exec PUBLIC ${THRIFT_INCLUDE_DIRS})
target_include_directories(server_exec PUBLIC ${THRIFT_INCLUDE_DIRS})
target_link_libraries(client_exec PUBLIC ${THRIFT_LIBRARIES})
target_link_libraries(server_exec PUBLIC ${THRIFT_LIBRARIES})