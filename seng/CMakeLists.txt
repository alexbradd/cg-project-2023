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
FetchContent_Declare(tinyobjloader
  GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader
  GIT_TAG v1.0.6
)
FetchContent_declare(yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp
  GIT_TAG 0.8.0
)
FetchContent_MakeAvailable(fmt glm tinyobjloader yaml-cpp)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)
# find_package(fmt REQUIRED)
# find_package(glm REQUIRED)

add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
  PRIVATE
    ./src/application.cpp
    ./src/components/camera.cpp
    ./src/components/scene_config_component_factory.cpp
    ./src/entity.cpp
    ./src/input_manager.cpp
    ./src/log.cpp
    ./src/rendering/buffer.cpp
    ./src/rendering/command_buffer.cpp
    ./src/rendering/debug_messenger.cpp
    ./src/rendering/device.cpp
    ./src/rendering/fence.cpp
    ./src/rendering/framebuffer.cpp
    ./src/rendering/glfw_window.cpp
    ./src/rendering/image.cpp
    ./src/rendering/mesh.cpp
    ./src/rendering/object_shader.cpp
    ./src/rendering/pipeline.cpp
    ./src/rendering/render_pass.cpp
    ./src/rendering/renderer.cpp
    ./src/rendering/shader_stage.cpp
    ./src/rendering/swapchain.cpp
    ./src/scene/scene.cpp
    ./src/scene/scene_graph.cpp
    ./src/transform.cpp
    ./src/utils.cpp
)
target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/
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
    glm::glm
    Vulkan::Vulkan
    Vulkan::Headers
    yaml-cpp
  PRIVATE
    glfw
    tinyobjloader
)
