cmake_minimum_required(VERSION 3.16)
project(online_store LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PQXX REQUIRED)

include_directories(include)

add_executable(online_store
  src/main.cpp
  src/Models.cpp
  src/Payments.cpp
  src/Users.cpp
  src/Utils.cpp
  src/DatabaseConnection.cpp
)

target_link_libraries(online_store PRIVATE pqxx)
