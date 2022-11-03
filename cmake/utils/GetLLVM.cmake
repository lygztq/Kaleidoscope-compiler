#######################################################
# Enhanced version of find llvm.
#
# Usage:
#   find_llvm(${USE_LLVM} [${COMPONENTS}]*)
#
# Provide variables:
# - LLVM_INCLUDE_DIRS
# - LLVM_LIBS
# - LLVM_DEFINITIONS
# - LLVM_VERSION

function(find_llvm)

  # search for a user-specified llvm
  if(LLVM_PATH)
    list(APPEND CMAKE_PREFIX_PATH LLVM_PATH)
  endif()

  # get components
  if(NOT "${ARGN}")
    set(LLVM_COMPONENTS "all")
  else()
    separate_arguments(ARGN)
    set(LLVM_COMPONENTS "${ARGN}")
  endif(NOT "${ARGN}")

  # find llvm
  find_package(LLVM REQUIRED CONFIG)
  message(STATUS "Find LLVM (version ${LLVM_PACKAGE_VERSION}) cmake config file at ${LLVM_DIR}")

  # find llvm-config
  if(WIN32)
    set(LLVM_CONFIG_NAME "llvm-config.exe")
  else()
    set(LLVM_CONFIG_NAME "llvm-config")
  endif()

  set(LLVM_CONFIG_PATH "${LLVM_TOOLS_BINARY_DIR}/${LLVM_CONFIG_NAME}")
  if(EXISTS "${LLVM_CONFIG_PATH}")
    message(STATUS "Find llvm-config in ${LLVM_CONFIG_PATH}, will use it to config llvm.")
  else()
    message(FATAL_ERROR "Cannot find llvm-config at ${LLVM_CONFIG_PATH}")
  endif()

  # find llvm prefix
  execute_process(COMMAND "${LLVM_CONFIG_PATH}" "--prefix"
    RESULT_VARIABLE CMD_EXIT_CODE
    OUTPUT_VARIABLE LLVM_PREFIX
    ERROR_VARIABLE CMD_ERROR_STR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(CMD_EXIT_CODE)
    message(FATAL_ERROR "llvm-config error: ${CMD_EXIT_CODE} ${CMD_ERROR_STR}")
  else()
    string(REPLACE "\\" "/" LLVM_PREFIX ${LLVM_PREFIX})
    message(STATUS "Find LLVM_ROOT at ${LLVM_PREFIX}")
  endif()

  execute_process(COMMAND "${LLVM_CONFIG_PATH}" "--libdir"
    RESULT_VARIABLE CMD_EXIT_CODE
    OUTPUT_VARIABLE LLVM_LIB_DIR
    ERROR_VARIABLE CMD_ERROR_STR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(CMD_EXIT_CODE)
    message(FATAL_ERROR "llvm-config error: ${CMD_EXIT_CODE} ${CMD_ERROR_STR}")
  else()
    string(REPLACE "\\" "/" LLVM_LIB_DIR ${LLVM_LIB_DIR})
  endif()

  execute_process(COMMAND "${LLVM_CONFIG_PATH}" "--system-libs"
    RESULT_VARIABLE CMD_EXIT_CODE
    OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
    ERROR_VARIABLE CMD_ERROR_STR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(CMD_EXIT_CODE)
    message(FATAL_ERROR "llvm-config error: ${CMD_EXIT_CODE} ${CMD_ERROR_STR}")
  endif()

  execute_process(COMMAND "${LLVM_CONFIG_PATH}" "--cxxflags"
    RESULT_VARIABLE CMD_EXIT_CODE
    OUTPUT_VARIABLE LLVM_CXX_FLAGS
    ERROR_VARIABLE CMD_ERROR_STR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(CMD_EXIT_CODE)
    message(FATAL_ERROR "llvm-config error: ${CMD_EXIT_CODE} ${CMD_ERROR_STR}")
  endif()

  execute_process(COMMAND "${LLVM_CONFIG_PATH}" "--libnames" "${LLVM_COMPONENTS}"
    RESULT_VARIABLE CMD_EXIT_CODE
    OUTPUT_VARIABLE LLVM_LIB_NAMES
    ERROR_VARIABLE CMD_ERROR_STR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(CMD_EXIT_CODE)
    message(FATAL_ERROR "llvm-config error: ${CMD_EXIT_CODE} ${CMD_ERROR_STR}")
  endif()

  # config llvm cxx flags
  string(REGEX MATCHALL "(^| )-D[A-Za-z0-9_]*" DEFS ${LLVM_CXX_FLAGS})
  set(LLVM_DEFINITIONS "")
  foreach(flag IN ITEMS ${DEFS})
    string(STRIP "${flag}" def)
    list(APPEND LLVM_DEFINITIONS "${def}")
  endforeach(flag IN ITEMS ${DEFS})

  # collect LLVM_LIBS
  set(LLVM_LIBS "")
  separate_arguments(LLVM_LIB_NAMES)
  foreach(libname IN ITEMS ${LLVM_LIB_NAMES})
    list(APPEND LLVM_LIBS "${LLVM_LIB_DIR}/${libname}")
  endforeach(libname IN ITEMS ${LLVM_LIB_NAMES})

  separate_arguments(LLVM_SYSTEM_LIBS)
  foreach(libname IN ITEMS ${LLVM_SYSTEM_LIBS})
    list(APPEND LLVM_LIBS "${libname}")
  endforeach(libname IN ITEMS ${LLVM_SYSTEM_LIBS})

  # outputs
  set(LLVM_INCLUDE_DIRS "${LLVM_INCLUDE_DIRS}" PARENT_SCOPE)
  set(LLVM_LIBS "${LLVM_LIBS}" PARENT_SCOPE)
  set(LLVM_DEFINITIONS "${LLVM_DEFINITIONS}" PARENT_SCOPE)
  set(LLVM_VERSION "${LLVM_PACKAGE_VERSION}" PARENT_SCOPE)

  message(STATUS "Found LLVM_INCLUDE_DIRS=" "${LLVM_INCLUDE_DIRS}")
  message(STATUS "Found LLVM_DEFINITIONS=" "${LLVM_DEFINITIONS}")
  message(STATUS "Found LLVM_LIBS=" "${LLVM_LIBS}")
  message(STATUS "Found LLVM_VERSION=" "${LLVM_VERSION}")

endfunction(find_llvm)
