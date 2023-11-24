project(froggo)

add_executable(${PROJECT_NAME} main.cpp)
target_compile_features(${PROJECT_NAME}
  PRIVATE
    cxx_std_17
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
target_link_libraries(${PROJECT_NAME} seng)
