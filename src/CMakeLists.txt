# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Project name, language and version
project(pix C)
set(PIX_VERSION "1.0")

# Set the C standard to C17
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Set the default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler flags for Release build
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -flto -march=native -mtune=native -funroll-loops -Wall -pedantic")

# Define the version as a preprocessor macro
ADD_DEFINITIONS("-DPIX_VERSION=\"${PIX_VERSION}\"")

# Find libexif
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEXIF REQUIRED libexif)

# Source files
set(SRCS pix/pix.c)

# Add the executable target
add_executable(pix ${SRCS})
target_include_directories(pix PRIVATE ${LIBEXIF_INCLUDE_DIRS})
target_link_libraries(pix PRIVATE ${LIBEXIF_LIBRARIES})
target_compile_options(pix PRIVATE ${LIBEXIF_CFLAGS_OTHER})

# Linker flags to strip the binary, and link with libexif
set_target_properties(pix PROPERTIES LINK_FLAGS "-s")

# Standard install step to install the executable
install(TARGETS pix
    RUNTIME DESTINATION bin
)
