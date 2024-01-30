project(froggo)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
  PRIVATE
    ./src/camera_orchestrator.cpp
    ./src/main.cpp
    ./src/test_controller.cpp
)
target_compile_features(${PROJECT_NAME}
  PRIVATE
    cxx_std_17
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
target_link_libraries(${PROJECT_NAME} seng)

if(EXISTS "${PROJECT_SOURCE_DIR}/shaders_src")
  message(STATUS "Building shaders...")

  find_package(Vulkan COMPONENTS glslc)
  find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

  file(GLOB_RECURSE SHADER_SRC
    "./shaders_src/*.frag"
    "./shaders_src/*.vert"
  )
  foreach(SHADER ${SHADER_SRC})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
    add_custom_command(
      OUTPUT ${SPIRV}
      DEPENDS ${SHADER}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
      COMMAND ${glslc_executable} ${SHADER} -o ${SPIRV}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
  endforeach()

  add_custom_target(
    shaders
    DEPENDS ${SPIRV_BINARY_FILES}
  )
  add_dependencies(${PROJECT_NAME} shaders)

  file(GLOB_RECURSE SHADER_CFG "./shaders_src/*.yml")
  foreach(CFG ${SHADER_CFG})
    get_filename_component(FILE_NAME ${CFG} NAME)
    set(COPIED_SHADER_CFG "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}")
    add_custom_command(
      OUTPUT ${COPIED_SHADER_CFG}
      DEPENDS ${CFG}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CFG}" "${COPIED_SHADER_CFG}"
    )
    list(APPEND SHADER_CONFIGS ${COPIED_SHADER_CFG})
  endforeach()
  add_custom_target(
    shader_config
    DEPENDS ${SHADER_CONFIGS}
  )
  add_dependencies(${PROJECT_NAME} shader_config)

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${PROJECT_BINARY_DIR}/shaders"
      "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
  )
endif()

macro(copy_dir dirname)
  if(EXISTS "${PROJECT_SOURCE_DIR}/${dirname}")
    message(STATUS "Copying ${dirname}...")
    add_custom_target(${dirname}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/${dirname}"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/${dirname}"
        "${PROJECT_BINARY_DIR}/${dirname}"
    )
    add_dependencies(${PROJECT_NAME} ${dirname})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${dirname}"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/${dirname}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${dirname}"
    )
  endif()
endmacro()

copy_dir("assets")
copy_dir("scenes")
