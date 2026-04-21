# CMakeList.txt : CMake project for Flux, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (Flux 
	"main.cpp"  
	"engine/Window.cpp" 
	"engine/Window.h"
	"engine/gui/viewport/viewport.cpp" 
	"engine/gui/viewport/viewport.h" 
	"engine/gui/viewport/ribbon.cpp" 
	"engine/gui/viewport/explorer.cpp" 
	"engine/gui/viewport/explorer.h"
	"engine/gui/viewport/output.cpp" 
	"engine/gui/viewport/properties.h" 
	"engine/gui/viewport/properties.cpp" 
	"engine/Model.h" 
	"engine/3DRenderer.h"
 "engine/OpenGLManager.cpp" "engine/3DRenderer.cpp" "engine/Model.cpp" "engine/mechanics/camera.h")

if(WIN32)
    set_target_properties(Flux PROPERTIES 
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/templates"
		DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/templates/Project_Templates"
		DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/templates/File_Templates"
		DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Flux PROPERTY CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)


target_link_libraries(Flux 
	PRIVATE 
	glfw 
	imgui::imgui
	glm::glm
	opengl32
	dwmapi
	glad::glad
	assimp::assimp
	imguizmo::imguizmo
)

target_include_directories(Flux PRIVATE 
    "engine"
    "engine/gui/viewport"
	"assets"
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(MSVC)
    target_compile_options(Flux PRIVATE $<$<CONFIG:Release>:/O2 /GL>)
    target_link_options(Flux PRIVATE $<$<CONFIG:Release>:/LTCG>)
else()
    target_compile_options(Flux PRIVATE $<$<CONFIG:Release>:-O3 -flto>)
    target_link_options(Flux PRIVATE $<$<CONFIG:Release>:-flto>)
endif()

if(NOT MSVC)
    set_target_properties(Flux PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif()