#
# CMakeLists.txt -- zip module
#
# Copyright (c) 2013-2016 David Demelier <markand@malikania.fr>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

cmake_minimum_required(VERSION 3.5)
project(zip)

set(ZIP_VERSION_MAJOR "1")
set(ZIP_VERSION_MINOR "0")
set(ZIP_VERSION_PATCH "0")
set(ZIP_VERSION "${ZIP_VERSION_MAJOR}.${ZIP_VERSION_MINOR}.${ZIP_VERSION_PATCH}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${zip_SOURCE_DIR}/cmake)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 ${CMAKE_CXX_FLAGS}")
endif ()

find_package(ZIP REQUIRED)
find_package(Doxygen QUIET)

add_subdirectory(gtest)

enable_testing()
add_executable(
    zip
    ${zip_SOURCE_DIR}/zip.hpp
    ${zip_SOURCE_DIR}/test/main.cpp
    ${zip_SOURCE_DIR}/INSTALL.md
    ${zip_SOURCE_DIR}/LICENSE.md
    ${zip_SOURCE_DIR}/README.md
)
target_link_libraries(zip gtest ${ZIP_LIBRARIES})
target_include_directories(zip PRIVATE ${zip_SOURCE_DIR} ${ZIP_INCLUDE_DIRS})
target_compile_definitions(zip PRIVATE DIRECTORY=\"${zip_SOURCE_DIR}/test/data/\")
add_test(NAME zip COMMAND zip)

if (DOXYGEN_FOUND)
    if (NOT DOXYGEN_DOT_FOUND)
        set(DOXYGEN_HAVE_DOT "NO")
        set(DOXYGEN_DOT_PATH "")
    else ()
        set(DOXYGEN_HAVE_DOT "YES")
        set(DOXYGEN_DOT_PATH "\"${DOXYGEN_DOT_EXECUTABLE}\"")
    endif ()

    configure_file(${zip_SOURCE_DIR}/doc/Doxyfile.in ${zip_BINARY_DIR}/Doxyfile)

    add_custom_target(
        doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${zip_BINARY_DIR}/Doxyfile
        SOURCES
            ${zip_SOURCE_DIR}/doc/Doxyfile.in
            ${zip_SOURCE_DIR}/doc/mainpage.cpp
        WORKING_DIRECTORY ${zip_SOURCE_DIR}
    )
endif ()
