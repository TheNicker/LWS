 cmake_minimum_required(VERSION 4.0)
 project("framework")

file(GLOB_RECURSE sourceFiles
    "./src/*.hpp"
    "./src/*.cpp"
)


set(CMAKE_CXX_STANDARD 23)

include_directories("./src/LWS/include")


add_executable(HybridWindowingSystem  ${sourceFiles})
target_link_options(HybridWindowingSystem PRIVATE /SUBSYSTEM:CONSOLE)
