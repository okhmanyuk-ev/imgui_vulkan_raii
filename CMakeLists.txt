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
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

target_include_directories(${PROJECT_NAME} PUBLIC src)

# imgui

file(GLOB IMGUI_SRC 
	imgui/*.cpp 
	imgui/*.h
)	
target_include_directories(${PROJECT_NAME} PUBLIC imgui)
add_library(imgui STATIC ${IMGUI_SRC})
target_link_libraries(${PROJECT_NAME} imgui)
set_property(TARGET imgui PROPERTY FOLDER ${LIBS_FOLDER})
