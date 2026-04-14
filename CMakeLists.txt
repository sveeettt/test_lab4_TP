cmake_minimum_required(VERSION 3.10)
project(MusicSalon)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Флаги покрытия для всех целей
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")

# Находим SQLite
find_package(SQLite3 REQUIRED)

# Подключаем GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Основная библиотека
add_library(MusicSalonLib STATIC
    src/Database.cpp
    src/Auth.cpp
    src/Queries.cpp
    src/Functions.cpp
)
target_include_directories(MusicSalonLib PUBLIC include)
target_link_libraries(MusicSalonLib ${SQLite3_LIBRARIES})

# Исполняемый файл приложения
add_executable(music_salon src/main.cpp)
target_link_libraries(music_salon MusicSalonLib)

# Исполняемый файл тестов
add_executable(run_tests tests/test_music_salon.cpp)
target_link_libraries(run_tests MusicSalonLib gtest_main)

# Регистрируем тесты
enable_testing()
add_test(NAME MusicSalonTests COMMAND run_tests)