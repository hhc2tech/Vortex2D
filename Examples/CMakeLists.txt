set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
if (APPLE)
	set(GLFW_VULKAN_STATIC ON  CACHE BOOL "" FORCE)
endif()
download_project(PROJ                glfw3
                 GIT_REPOSITORY      https://github.com/glfw/glfw.git
                 GIT_TAG             3.2.1
                 ${UPDATE_DISCONNECTED_IF_AVAILABLE})
add_subdirectory(${glfw3_SOURCE_DIR} ${glfw3_BINARY_DIR})

download_project(PROJ                Box2D
                 GIT_REPOSITORY      https://github.com/erincatto/Box2D.git
                 GIT_TAG             v2.3.1
                 ${UPDATE_DISCONNECTED_IF_AVAILABLE})

set(BOX2D_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${Box2D_SOURCE_DIR}/Box2D ${Box2D_BINARY_DIR})

file(GLOB EXAMPLES_SOURCES
        "main.cpp"
        "RenderExample.h"
        "ObstacleSmokeExample.h"
        "SmokeExample.h"
        "SmokeVelocityExample.h"
        "WaterExample.h"
        "BuoyancyWaterExample.h"
        "WaterFallExample.h"
        "WatermillExample.h"
        "Runner.h")

file(GLOB EXAMPLES_BOX2D_SOURCES
        "Rigidbody.h"
        "Rigidbody.cpp")

# Build Box2D stuff separately
add_library(vortex2d_examples_box2d STATIC ${EXAMPLES_BOX2D_SOURCES})
target_include_directories(vortex2d_examples_box2d PUBLIC ${Box2D_SOURCE_DIR}/Box2D)
target_link_libraries(vortex2d_examples_box2d PUBLIC vortex2d Box2D)

add_executable(vortex2d_examples ${EXAMPLES_SOURCES})
target_link_libraries(vortex2d_examples vortex2d glfw vortex2d_examples_box2d)

if (WIN32)
    vortex2d_copy_dll(vortex2d_examples)
endif()
