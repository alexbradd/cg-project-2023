project(seng LANGUAGES CXX)
message(STATUS "Building ${PROJECT_NAME}")

message(STATUS "Fetching dependencies...")
include(FetchContent)
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 10.1.1
)
FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm
  GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(fmt glm)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)
# find_package(fmt REQUIRED)
# find_package(glm REQUIRED)

add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
  PRIVATE
    ./src/application.cpp
    ./src/glfw_window.cpp
    ./src/utils.cpp
    # ./src/vulkan_internals.cpp
    ./src/vulkan_renderer.cpp
    ./src/vulkan_swapchain.cpp
    ./src/vulkan_swapchain_support_details.cpp
    ./src/vulkan_queue_family_indices.cpp
    ./src/vulkan_debug_messenger.cpp
    ./src/vulkan_device.cpp
)
target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/public
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include/private
)
target_compile_options(${PROJECT_NAME}
  PRIVATE
    -Wall
    -Wextra
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
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    fmt::fmt
  PRIVATE
    glfw
    Vulkan::Vulkan
    Vulkan::Headers
    glm::glm
)
