file(GLOB PARSER_SOURCE_LIST "${CMAKE_SOURCE_DIR}/src/parser/*.cc")
list(REMOVE_ITEM PARSER_SOURCE_LIST "${CMAKE_SOURCE_DIR}/src/parser/parser_exec.cc")
add_library(klang_parser_lib SHARED ${PARSER_SOURCE_LIST})
add_dependencies(klang_parser_lib klang_lexer_lib)
set_target_properties(klang_parser_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(klang_parser_lib PROPERTIES DEFINE_SYMBOL PARSER_EXPORT)
target_link_directories(klang_parser_lib PRIVATE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                                         PRIVATE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                                         PRIVATE ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
target_link_libraries(klang_parser_lib klang_lexer_lib)
target_include_directories(klang_parser_lib PRIVATE "${CMAKE_SOURCE_DIR}/src/parser")

add_executable(klang_parser "${CMAKE_SOURCE_DIR}/src/parser/parser_exec.cc")
add_dependencies(klang_parser klang_parser_lib klang_lexer_lib)
target_link_directories(klang_parser PRIVATE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                                     PRIVATE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                                     PRIVATE ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
target_link_libraries(klang_parser klang_parser_lib klang_lexer_lib)
