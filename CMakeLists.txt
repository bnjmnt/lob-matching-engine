cmake_minimum_required(VERSION 3.28)
project(lob_matching_engine CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

add_library(lob_warnings INTERFACE)
target_compile_options(lob_warnings INTERFACE
  -Wall
  -Wextra
  -Wpedantic
)

add_library(lob_core
  src/order_pool.cpp
  src/price_level.cpp
  src/order_book.cpp
)
target_include_directories(lob_core PUBLIC include)
target_link_libraries(lob_core PRIVATE lob_warnings)

add_executable(lob_main src/main.cpp)
target_link_libraries(lob_main PRIVATE lob_core lob_warnings)

add_subdirectory(tests)