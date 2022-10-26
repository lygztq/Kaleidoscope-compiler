file(GLOB LEXER_SOURCE_LIST "${CMAKE_SOURCE_DIR}/src/lexer/*.cc")
list(REMOVE_ITEM LEXER_SOURCE_LIST "${CMAKE_SOURCE_DIR}/src/lexer/lexer_exec.cc")
add_library(klang_lexer_lib SHARED ${LEXER_SOURCE_LIST})
set_target_properties(klang_lexer_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(klang_lexer_lib PROPERTIES DEFINE_SYMBOL LEXER_EXPORT)
target_include_directories(klang_lexer_lib PRIVATE "${CMAKE_SOURCE_DIR}/src/lexer")

add_executable(klang_lexer "${CMAKE_SOURCE_DIR}/src/lexer/lexer_exec.cc")
add_dependencies(klang_lexer klang_lexer_lib)
target_link_directories(klang_lexer PRIVATE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                                    PRIVATE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                                    PRIVATE ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
target_link_libraries(klang_lexer klang_lexer_lib)
target_include_directories(klang_lexer PRIVATE "${CMAKE_SOURCE_DIR}/src/lexer")
