cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    morninglang
    VERSION 0.1.0
    DESCRIPTION "A Programming Language in C++ and LLVM"
    HOMEPAGE_URL "https://github.com/alexeev-prog/morning.lang"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

find_package(LLVM REQUIRED CONFIG)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finput-charset=UTF-8 -Waddress -O1 -pedantic-errors -Wall -Wextra -Wpedantic -Wcast-align -Wcast-qual -Wconversion -Wctor-dtor-privacy -Wextra-semi -Wfloat-equal -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-conversion")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})

# ---- Declare library ----

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs support core irreader)

add_library(
    morninglang_lib OBJECT
    source/tracelogger.cpp
    source/morningllvm.hpp
    source/parser/MorningLangGrammar.h
)
target_link_libraries(morninglang_lib ${llvm_libs})

target_include_directories(
    morninglang_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(morninglang_lib PUBLIC cxx_std_23)

# ---- Declare executable ----

add_executable(morninglang_exe source/main.cpp)
add_executable(morninglang::exe ALIAS morninglang_exe)

set_property(TARGET morninglang_exe PROPERTY OUTPUT_NAME morninglang)

target_compile_features(morninglang_exe PRIVATE cxx_std_17)

target_link_libraries(morninglang_exe PRIVATE morninglang_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT morninglang_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of morninglang"
  )
endif()

include(cmake/dev-mode.cmake)
