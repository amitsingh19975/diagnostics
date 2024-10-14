find_package(Catch2 CONFIG REQUIRED)

set(CTEST_OUTPUT_ON_FAILURE 1)
include(CTest)
include(Catch)
add_library(test_lib INTERFACE)
target_link_libraries(test_lib INTERFACE Catch2::Catch2 Catch2::Catch2WithMain)
# set(CMAKE_CXX_FLAGS "-fsanitize=address -fno-omit-frame-pointer")

function(add_catch_test source_filename)
    message(STATUS "Adding test: ${source_filename}")
    get_filename_component(target ${source_filename} NAME_WE)
    add_executable(${target} ${source_filename})
    target_link_libraries(${target} PRIVATE test_lib diagnostics_core ${llvm_libs})
    catch_discover_tests(${target} TEST_PREFIX "unittests." EXTRA_ARGS -s --reporter=xml --out=tests.xml)
endfunction(add_catch_test target)

add_subdirectory(runtime)
