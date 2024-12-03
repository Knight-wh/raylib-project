get_filename_component(FOLODER_NAME "${CMAKE_CURRENT_LIST_DIR}" NAME)

add_executable(${FOLODER_NAME})
target_sources(${FOLODER_NAME} PRIVATE main.cpp)
target_link_libraries(${FOLODER_NAME} PRIVATE raylib)

target_compile_definitions(${FOLODER_NAME} PUBLIC ASSETS_PATH="${CMAKE_SOURCE_DIR}/assets/")