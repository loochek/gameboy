cmake_minimum_required(VERSION 3.1)

project(gb)

# set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
# set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

aux_source_directory(src/core GB_CORE_SOURCES)
aux_source_directory(src/frontends/sfml GB_SFML_SOURCES)
aux_source_directory(src/frontends/libretro GB_LIBRETRO_SOURCES)

find_package(SFML COMPONENTS graphics window)
if (SFML_FOUND)
    add_executable(gb ${GB_CORE_SOURCES} ${GB_SFML_SOURCES})
    target_include_directories(gb PUBLIC src/core src/frontends/sfml)
    target_link_libraries(gb csfml-graphics csfml-window)
endif()

add_library(gb_libretro SHARED ${GB_CORE_SOURCES} ${GB_LIBRETRO_SOURCES})
set_target_properties(gb_libretro PROPERTIES PREFIX "")
target_link_options(gb_libretro PUBLIC -Wl,--version-script=${CMAKE_SOURCE_DIR}/link.T)
target_include_directories(gb_libretro PUBLIC src/core src/frontends/libretro)
