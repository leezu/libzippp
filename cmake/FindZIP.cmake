# FindZIP
# -------
#
# Find libzip library, this modules defines:
#
# ZIP_INCLUDE_DIRS, where to find zip.h
# ZIP_LIBRARIES, where to find library
# ZIP_FOUND, if it is found

find_package(ZLIB QUIET)

find_path(
	ZIP_INCLUDE_DIR
	NAMES zip.h
)

find_library(
	ZIP_LIBRARY
	NAMES zip libzip
)

find_path(
	ZIPCONF_INCLUDE_DIR
	NAMES zipconf.h
)

if (NOT ZIPCONF_INCLUDE_DIR)
	# zipconf.h is sometimes directly in the include/ folder but on some systems
	# like Windows, it is installed in the lib/ directory.
	get_filename_component(_ZIP_PRIVATE_LIBRARY "${ZIP_LIBRARY}" DIRECTORY)

	find_path(
		ZIPCONF_INCLUDE_DIR
		NAMES zipconf.h
		PATHS "${_ZIP_PRIVATE_LIBRARY}/libzip/include"
	)
endif ()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
	ZIP
	REQUIRED_VARS ZLIB_LIBRARIES ZLIB_INCLUDE_DIRS ZIP_LIBRARY ZIP_INCLUDE_DIR ZIPCONF_INCLUDE_DIR
)

if (ZIP_FOUND)
	set(ZIP_LIBRARIES ${ZIP_LIBRARY} ${ZLIB_LIBRARIES})
	set(ZIP_INCLUDE_DIRS ${ZIP_INCLUDE_DIR} ${ZIPCONF_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS})
endif ()

mark_as_advanced(ZIP_LIBRARY ZIP_INCLUDE_DIR ZIPCONF_INCLUDE_DIR)
