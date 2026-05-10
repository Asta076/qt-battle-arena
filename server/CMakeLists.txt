cmake_minimum_required(VERSION 3.16)
project(BattleArenaServer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost 1.80 REQUIRED)
find_package(Threads REQUIRED)

add_executable(BattleArenaServer main.cpp)

target_include_directories(BattleArenaServer PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(BattleArenaServer PRIVATE
    Threads::Threads
    ${Boost_LIBRARIES}
)

# Windows needs winsock
if(WIN32)
    target_link_libraries(BattleArenaServer PRIVATE ws2_32 mswsock)
endif()