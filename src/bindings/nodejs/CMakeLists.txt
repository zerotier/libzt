cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)

project (zts)

add_definitions(-DNAPI_VERSION=6)

include_directories(${CMAKE_JS_INC})


file(GLOB SOURCE_FILES "src/native/binding.cc")


add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})

# link libzt library
set(ZTS_ENABLE_RUST TRUE)
add_subdirectory(libzt-root)
target_link_libraries(${PROJECT_NAME} zt-static)
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/libzt-root/include")

if(MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
  # Generate node.lib
  execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
endif()

# Include N-API wrappers
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/node_modules/node-addon-api")

