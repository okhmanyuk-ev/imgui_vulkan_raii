cmake_minimum_required(VERSION 3.10)
project(imgui_vulkan_raii)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRC
	src/*.cpp
	src/*.h
)

add_executable(${PROJECT_NAME} 
	${SRC}
)

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
#set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

target_include_directories(${PROJECT_NAME} PUBLIC src)

# imgui

file(GLOB IMGUI_SRC 
	lib/imgui/*.cpp 
	lib/imgui/*.h
)
target_include_directories(${PROJECT_NAME} PUBLIC lib/imgui)
add_library(imgui STATIC ${IMGUI_SRC})
target_link_libraries(${PROJECT_NAME} imgui)

# vulkan

add_library(vulkan INTERFACE)
target_link_libraries(vulkan INTERFACE ${PROJECT_SOURCE_DIR}/lib/vulkan/lib/vulkan-1.lib)
target_include_directories(vulkan INTERFACE lib/vulkan/include)
if(WIN32)
	target_compile_definitions(vulkan INTERFACE -DVK_USE_PLATFORM_WIN32_KHR)
endif()
target_link_libraries(${PROJECT_NAME} vulkan)

# glfw

set(GLFW_INSTALL OFF)
add_subdirectory(lib/glfw)
target_link_libraries(${PROJECT_NAME} glfw)
