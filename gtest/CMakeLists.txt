#
# CMakeLists.txt -- googletest library
#
# Copyright (c) 2013-2017 David Demelier <markand@malikania.fr>
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

project(GoogleTest)

add_library(gtest src/gtest-all.cc)

target_include_directories(
	gtest
	PUBLIC
		${GoogleTest_SOURCE_DIR}/include
	PRIVATE
		${GoogleTest_SOURCE_DIR}
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(gtest PUBLIC -Wno-missing-field-initializers -Wno-unused-variable)
endif ()

if (NOT WIN32)
	target_link_libraries(gtest -pthread)
endif ()
