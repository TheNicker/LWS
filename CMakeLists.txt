cmake_minimum_required(VERSION 3.20)
project("LWS")

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE sourceFiles
    "./src/*.hpp"
    "./src/*.cpp"
)

add_executable(LWS ${sourceFiles})
target_include_directories(LWS PRIVATE "./src")
target_include_directories(LWS PRIVATE "./src/LWS/include")
target_include_directories(LWS PRIVATE "./external/LLUtils/include")
target_compile_definitions(LWS PRIVATE NOMINMAX UNICODE _UNICODE LWS_PLATFORM_WIN32)
target_link_options(LWS PRIVATE /SUBSYSTEM:CONSOLE)

if(WIN32)
  target_compile_definitions(LWS PUBLIC LWS_PLATFORM_WIN32)
  target_link_libraries(LWS PRIVATE
      Ole32
      Shell32
      Shcore
      User32
      Gdi32
      ComCtl32
  )
elseif(UNIX)
  find_package(Wayland QUIET)
  if(Wayland_FOUND)
    target_compile_definitions(LWS PUBLIC LWS_PLATFORM_WAYLAND)
  else()
    find_package(X11 REQUIRED)
    target_compile_definitions(LWS PUBLIC LWS_PLATFORM_X11)
  endif()
endif()