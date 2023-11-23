project(seng LANGUAGES CXX)

message(STATUS "Building ${PROJECT_NAME}")

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(fmt REQUIRED)

add_library(${PROJECT_NAME}
  src/application.cpp
  src/vulkan_internals.cpp
)
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
  PRIVATE
    ${PROJECT_SOURCE_DIR}/private
)
target_link_libraries(${PROJECT_NAME}
  PRIVATE
    glfw
    Vulkan::Vulkan
    Vulkan::Headers
    fmt::fmt
)
