add_executable(${PROJECT_NAME} test.cpp)
target_link_libraries(
  ${PROJECT_NAME} PRIVATE project_options project_warnings diagnostics_core)
