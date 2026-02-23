cmake_minimum_required(VERSION 3.29)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(HelloTriangle)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")

find_package(glfw3 REQUIRED)
# find_package(glm REQUIRED)
add_subdirectory(external/glm)
find_package(Vulkan 1.4.335 REQUIRED)

add_executable(HelloTriangle main.cpp)
set_target_properties(HelloTriangle PROPERTIES CXX_STANDARD 20)
target_compile_definitions(
  HelloTriangle
  PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
          VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
          VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
          VULKAN_HPP_NO_SPACESHIP_OPERATOR)
target_include_directories(HelloTriangle PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(HelloTriangle PRIVATE Vulkan::Vulkan glfw glm::glm)

if(WIN32)
  if(CMAKE_GENERATOR MATCHES "Visual Studio.*")
    set_target_properties(HelloTriangle PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY
                                                   "${CMAKE_BINARY_DIR}")
  endif()
endif()

# =============================================================================
# Copy compile_commands.json (Development convenience)
# =============================================================================

# Only do this for the main project, not when used as submodule
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_BINARY_DIR}/compile_commands.json"
      "${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json"
    COMMENT "Copying compile_commands.json to source directory")
endif()
