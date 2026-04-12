# CMakeList.txt : CMake project for Flux, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (Flux "main.cpp"  "engine/Window.cpp" "engine/Window.h" "engine/gui/viewport/viewport.cpp" "engine/gui/viewport/viewport.h")

if(WIN32)
    set_target_properties(Flux PROPERTIES 
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

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
)

# TODO: Add tests and install targets if needed.
