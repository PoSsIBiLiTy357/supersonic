ExternalProject_Add(gtest
    GIT_REPOSITORY "https://github.com/google/googletest"
    GIT_TAG "release-1.8.0"
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS
    	<BINARY_DIR>/googlemock/gtest/libgtest.a
    	<BINARY_DIR>/googlemock/gtest/libgtest_main.a
    	<BINARY_DIR>/googlemock/libgmock.a
    	<BINARY_DIR>/googlemock/libgmock_main.a
)

# Get GTest source and binary directories from CMake project
ExternalProject_Get_Property(gtest source_dir binary_dir)

# Workarounds
file(MAKE_DIRECTORY "${source_dir}/googletest/include")
file(MAKE_DIRECTORY "${source_dir}/googlemock/include")

# Create a libgtest target to be used as a dependency by test programs
add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)

# Set libgtest properties
set_target_properties(libgtest PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/googlemock/gtest/libgtest.a"
    "INTERFACE_INCLUDE_DIRECTORIES" "${source_dir}/googletest/include"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
)

# Create a libgtest target to be used as a dependency by test programs
add_library(libgtest_main IMPORTED STATIC GLOBAL)
add_dependencies(libgtest_main gtest_main)

# Set libgtest properties
set_target_properties(libgtest_main PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/googlemock/gtest/libgtest_main.a"
    "INTERFACE_INCLUDE_DIRECTORIES" "${source_dir}/googletest/include"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
)

# Create a libgmock target to be used as a dependency by test programs
add_library(libgmock IMPORTED STATIC GLOBAL)
add_dependencies(libgmock libgtest)

# Set libgmock properties
set_target_properties(libgmock PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/googlemock/libgmock.a"
    "INTERFACE_INCLUDE_DIRECTORIES" "${source_dir}/googlemock/include"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
)
