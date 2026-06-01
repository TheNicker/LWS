 cmake_minimum_required(VERSION 4.0)
 project("framework")

file(GLOB_RECURSE sourceFiles
    "./src/*.hpp"
    "./src/*.cpp"
)


set(CMAKE_CXX_STANDARD 23)






add_executable(HybridWindowingSystem  ${sourceFiles})
target_include_directories(HybridWindowingSystem PRIVATE "./src/LWS/include")
target_include_directories(HybridWindowingSystem PRIVATE "./external/LLUtils/include")
target_include_directories(HybridWindowingSystem PRIVATE "./external/Win32/Win32/Include")
target_compile_definitions(HybridWindowingSystem PRIVATE NOMINMAX)

target_link_options(HybridWindowingSystem PRIVATE /SUBSYSTEM:CONSOLE)
