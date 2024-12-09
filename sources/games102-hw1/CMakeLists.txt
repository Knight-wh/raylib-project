get_filename_component(FOLODER_NAME "${CMAKE_CURRENT_LIST_DIR}" NAME)

add_executable(${FOLODER_NAME})
target_sources(${FOLODER_NAME} PRIVATE main.cpp)
target_link_libraries(${FOLODER_NAME} PRIVATE raylib)

target_compile_definitions(${FOLODER_NAME} PUBLIC ASSETS_PATH="${CMAKE_SOURCE_DIR}/assets/")
target_compile_options(${FOLODER_NAME} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj> $<$<CXX_COMPILER_ID:GNU>:-Wa,-mbig-obj>)