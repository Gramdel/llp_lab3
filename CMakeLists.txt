cmake_minimum_required (VERSION 3.8)

project ("llp_lab1")

# Add source to this project's executable.
add_executable (llp_lab1 "src/main.c" "src/zgdb/format.c" "src/zgdb/format.h" "src/zgdb/document.h" "src/zgdb/document.c" "src/zgdb/list.c" "src/zgdb/list.h" "src/utils/optional.h" src/utils/optional.c src/zgdb/document_public.h src/zgdb/format_public.h src/zgdb/element.c src/zgdb/element.h src/zgdb/element_public.h src/zgdb/query.h src/zgdb/query.c)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET llp_lab1 PROPERTY C_STANDARD 17)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mno-ms-bitfields")
endif()