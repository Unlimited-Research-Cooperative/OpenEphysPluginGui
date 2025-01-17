# Open Ephys GUI main build file
cmake_minimum_required(VERSION 3.15)

set(GUI_VERSION 0.6.6)

string(REGEX MATCHALL "[0-9]+" VERSION_LIST ${GUI_VERSION})
set(GUI_VERSION_HEX "0x")
foreach(_v ${VERSION_LIST})
    if (NOT ${_v} STREQUAL "0")
        string(APPEND GUI_VERSION_HEX "0${_v}")
    endif()
endforeach()
string(REPLACE "." "," GUI_VERSION_COMMA ${GUI_VERSION})

# Ensure the build is being run from the correct directory
if ((NOT ${CMAKE_CURRENT_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR}/Build ) AND (NOT OE_DONT_CHECK_BUILD_PATH))
    message(FATAL_ERROR "Please run cmake inside the Build folder to build the binaries in the predefined paths.\nSet the OE_DONT_CHECK_BUILD_PATH cmake variable to TRUE to disable this error")
endif()

project(open-ephys-GUI)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(LINUX 1)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug)
    endif()
endif()
include(HelperFunctions.cmake)

set(BASE_BUILD_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Build)
set(JUCE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/JuceLibraryCode)
set(RESOURCES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Resources)

configure_file(${JUCE_DIRECTORY}/JuceHeader.h.in ${JUCE_DIRECTORY}/JuceHeader.h)
configure_file(${RESOURCES_DIRECTORY}/Build-files/resources.rc.in ${RESOURCES_DIRECTORY}/Build-files/resources.rc)

# Global properties
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
    $<$<CONFIG:Debug>:DEBUG=1>
    $<$<CONFIG:Debug>:_DEBUG=1>
    $<$<CONFIG:Release>:NDEBUG=1>
    JUCE_APP_VERSION=${GUI_VERSION}
    JUCE_APP_VERSION_HEX=${GUI_VERSION_HEX}
)

# Set file extension for JUCE related files
set(JUCE_FILES_EXTENSION cpp)

# Create executable and add JUCE components
add_executable(open-ephys
    ${JUCE_DIRECTORY}/AppConfig.h
    ${JUCE_DIRECTORY}/JuceHeader.h
    ${JUCE_DIRECTORY}/BinaryData.h
    ${JUCE_DIRECTORY}/BinaryData.cpp
    ${JUCE_DIRECTORY}/include_juce_audio_basics.${JUCE_FILES_EXTENSION}
    ${JUCE_DIRECTORY}/include_juce_audio_devices.${JUCE_FILES_EXTENSION}
    ${JUCE_DIRECTORY}/include_juce_audio_formats.${JUCE_FILES_EXTENSION}
    ${JUCE_DIRECTORY}/include_juce_audio_processors.${JUCE_FILES_EXTENSION}
    ${JUCE_DIRECTORY}/include_juce_audio_utils.${JUCE_FILES_EXTENSION}
    ${JUCE_DIRECTORY}/include_juce_core.${JUCE_FILES_EXTENSION}
    ${JUCE_DIRECTORY}/include_juce_cryptography.${JUCE_FILES_EXTENSION}
    ${JUCE_DIRECTORY}/include_juce_data_structures.${JUCE_FILES_EXTENSION}
    ${JUCE_DIRECTORY}/include_juce_events.${JUCE_FILES_EXTENSION}
    ${JUCE_DIRECTORY}/include_juce_graphics.${JUCE_FILES_EXTENSION}
    ${JUCE_DIRECTORY}/include_juce_gui_basics.${JUCE_FILES_EXTENSION}
    ${JUCE_DIRECTORY}/include_juce_gui_extra.${JUCE_FILES_EXTENSION}
    ${JUCE_DIRECTORY}/include_juce_opengl.${JUCE_FILES_EXTENSION}
    ${JUCE_DIRECTORY}/include_juce_video.${JUCE_FILES_EXTENSION}
)
if(MSVC)
    target_sources(open-ephys PRIVATE ${RESOURCES_DIRECTORY}/Build-files/resources.rc)
endif()

target_include_directories(open-ephys PRIVATE ${JUCE_DIRECTORY} ${JUCE_DIRECTORY}/modules)
target_compile_features(open-ephys PUBLIC cxx_auto_type cxx_generalized_initializers cxx_std_17)

# Find and Link ZeroMQ
find_package(ZeroMQ REQUIRED)
target_link_libraries(open-ephys PRIVATE libzmq)
target_include_directories(open-ephys PRIVATE ${ZeroMQ_INCLUDE_DIRS})

# Additional libraries and include directories
target_include_directories(open-ephys PRIVATE
    /usr/include
    /usr/include/freetype2
)

target_link_libraries(open-ephys
    PRIVATE
    GL X11 Xext Xinerama asound dl freetype pthread rt
)

# Find additional required packages
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WEBKIT2GTK REQUIRED webkit2gtk-4.0)

target_include_directories(open-ephys PRIVATE
    ${CURL_INCLUDE_DIRS}
    ${WEBKIT2GTK_INCLUDE_DIRS}
)

target_link_libraries(open-ephys
    PRIVATE
    ${CURL_LIBRARIES}
    ${WEBKIT2GTK_LIBRARIES}
)

# Output folders and specific options for Linux
set(BIN_PLUGIN_DIR $<TARGET_FILE_DIR:open-ephys>/plugins)
set(BIN_SHARED_DIR $<TARGET_FILE_DIR:open-ephys>/shared)
add_custom_command(TARGET open-ephys POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_SHARED_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_PLUGIN_DIR}"
)

# Copy bitfiles
add_custom_command(TARGET open-ephys POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${_bitfiles} "${BIN_SHARED_DIR}"
)

# Copy default configs and their icons
add_custom_command(TARGET open-ephys POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:open-ephys>/configs
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:open-ephys>/configs/icons
    COMMAND ${CMAKE_COMMAND} -E copy ${_xmlfiles} $<TARGET_FILE_DIR:open-ephys>/configs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${RESOURCES_DIRECTORY}/Configs/Icons $<TARGET_FILE_DIR:open-ephys>/configs/icons
)

# Copy FileReader resources to output directory
add_custom_command(TARGET open-ephys POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${RESOURCES_DIRECTORY}/FileReader/resources $<TARGET_FILE_DIR:open-ephys>/resources
)

# Copy GUI icon to output directory
add_custom_command(TARGET open-ephys POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${RESOURCES_DIRECTORY}/Icons/icon-small.png $<TARGET_FILE_DIR:open-ephys>/
)

# Linux Specific Settings
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    target_compile_options(open-ephys PRIVATE -O0) # Enable optimization for Linux debug
elseif (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    target_compile_options(open-ephys PRIVATE -O3) # Enable optimization for Linux release
else()
    message(FATAL_ERROR "Invalid build type")
endif()

target_include_directories(open-ephys PRIVATE
    /usr/include
    /usr/include/freetype2
)

target_link_libraries(open-ephys PRIVATE
    GL X11 Xext Xinerama asound dl freetype pthread rt 
    ${CURL_LIBRARIES} ${WEBKIT2GTK_LIBRARIES}
)

set_property(TARGET open-ephys APPEND_STRING PROPERTY LINK_FLAGS
    "-fvisibility=hidden -fPIC -rdynamic -Wl,-rpath,'$ORIGIN/shared'"
)

target_compile_options(open-ephys PRIVATE -fPIC -rdynamic -fvisibility=hidden -Wno-free-nonheap-object)
set_property(TARGET open-ephys PROPERTY RUNTIME_OUTPUT_DIRECTORY ${BASE_BUILD_DIRECTORY}/${CMAKE_BUILD_TYPE})

# Start recursive directory add
add_subdirectory(Source)

# Create filters for VS and Xcode
get_target_property(SRC_FILES open-ephys SOURCES)

foreach(src_file IN ITEMS ${SRC_FILES})
    get_filename_component(src_path "${src_file}" PATH)
    file(RELATIVE_PATH src_path_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${src_path}")
    string(REPLACE "/" "\\" group_name "${src_path_rel}")
    source_group("${group_name}" FILES "${src_file}")
endforeach()

# Add plugin build files
add_subdirectory(Plugins)