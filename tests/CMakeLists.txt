include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(lob_tests
  order_test.cpp
  order_pool_test.cpp
  price_level_test.cpp
  order_book_test.cpp
)
target_link_libraries(lob_tests PRIVATE lob_core lob_warnings GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(lob_tests)