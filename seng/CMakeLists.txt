project(seng LANGUAGES CXX)

message(STATUS "Building ${PROJECT_NAME}")

add_library(${PROJECT_NAME} src/seng.cpp)
target_compile_options(${PROJECT_NAME}
  PRIVATE
    -Wall
  PUBLIC
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:RelWithDebInfo>:-O2 -g>
    $<$<CONFIG:Release>:-O2>
    $<$<CONFIG:MinSizeRel>:-O2>
)
target_compile_features(${PROJECT_NAME}
  PRIVATE
    cxx_std_17
)
target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
