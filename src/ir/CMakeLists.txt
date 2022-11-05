file(GLOB IR_SOURCE_LIST "${CMAKE_SOURCE_DIR}/src/ir/*.cc")
add_library(kaleidoscope_ir_lib SHARED ${IR_SOURCE_LIST})
target_include_directories(kaleidoscope_ir_lib
                            PUBLIC  "${LLVM_INCLUDE_DIRS}"
                            PRIVATE "${CMAKE_SOURCE_DIR}/src/ir")

set(IR_LINKED_LIBS "${LLVM_LIBS}")
list(APPEND IR_LINKED_LIBS klang_lexer_lib klang_parser_lib)

add_dependencies(kaleidoscope_ir_lib klang_lexer_lib klang_parser_lib)
target_link_directories(kaleidoscope_ir_lib PRIVATE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                                            PRIVATE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                                            PRIVATE ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
target_link_libraries(kaleidoscope_ir_lib ${IR_LINKED_LIBS})
set_target_properties(kaleidoscope_ir_lib PROPERTIES
                      # make it compitible with LLVM-prebuild version
                      CXX_STANDARD ${LLVM_CXX_STANDARD}
                      POSITION_INDEPENDENT_CODE ON
                      DEFINE_SYMBOL IR_EXPORT)
target_compile_definitions(kaleidoscope_ir_lib PRIVATE "${LLVM_DEFINITIONS}")
