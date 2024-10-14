function(enable_sanitizers project_name)
    
    set(SANITIZERS "")

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "^(Apple)?Clang$")
        option(ENABLE_CONVERAGE "Enable coverage reporting for gcc/clang" OFF)
        if (ENABLE_CONVERAGE)
            target_compile_options(${project_name} INTERFACE --coverage -O0 -g)
            target_link_libraries(${project_name} INTERFACE --coverage)
        endif(ENABLE_CONVERAGE)

        option(ENABLE_SANITIZER_ASAN "Enable address sanitizer" OFF)
        option(ENABLE_SANITIZER_UBSAN "Enable undefined behavior sanitizer" OFF)
        option(ENABLE_SANITIZER_TSAN "Enable thread sanitizer" OFF)
        option(ENABLE_SANITIZER_MSAN "Enable memory sanitizer" OFF)

        if (ENABLE_SANITIZER_ASAN)
            list(APPEND SANITIZERS "address")
        endif(ENABLE_SANITIZER_ASAN)

        if (ENABLE_SANITIZER_UBSAN)
            list(APPEND SANITIZERS "undefined")
        endif(ENABLE_SANITIZER_UBSAN)

        if (ENABLE_SANITIZER_TSAN)
            list(APPEND SANITIZERS "thread")
        endif(ENABLE_SANITIZER_TSAN)

        if (ENABLE_SANITIZER_MSAN)
            list(APPEND SANITIZERS "memory")
        endif(ENABLE_SANITIZER_MSAN)

    endif()
    
    list(JOIN SANITIZERS "," SANITIZERS_STRING)

    if (NOT "S{SANITIZERS_STRING}" STREQUAL "")
        target_compile_options(${project_name} INTERFACE -fsanitize=${SANITIZERS_STRING})
        target_link_libraries(${project_name} INTERFACE -fsanitize=${SANITIZERS_STRING})
    endif()

endfunction(enable_sanitizers project_name)
