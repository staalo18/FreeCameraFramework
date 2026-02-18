# It's recommended to set a minimum CMake version.
# If you use CMake features from higher versions, update this to match.
cmake_minimum_required(VERSION 3.21)

# Set your project name. This will be the name of your SKSE .dll file.
set(NAME "FreeCameraFramework" CACHE STRING "")
set(VERSION 0.9.5 CACHE STRING "")

project(
	${NAME}
	VERSION ${VERSION}
	LANGUAGES CXX
)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/Plugin.h.in
	${CMAKE_CURRENT_BINARY_DIR}/include/Plugin.h
	@ONLY
)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/plugin.rc.in
	${CMAKE_CURRENT_BINARY_DIR}/plugin.rc
	@ONLY
)

list(APPEND SOURCES ${CMAKE_CURRENT_BINARY_DIR}/plugin.rc)

set(OUTPUT_FOLDER "C:/Users/tom/AppData/Local/ModOrganizer/MO_SkyrimSE/mods/_DEV_FreeCameraFramework")

option(ENABLE_SKYRIM_SE "Enable support for Skyrim SE in the dynamic runtime feature." ON)
option(ENABLE_SKYRIM_AE "Enable support for Skyrim AE in the dynamic runtime feature." ON)
option(ENABLE_SKYRIM_VR "Enable support for Skyrim VR in the dynamic runtime feature." OFF)
set(BUILD_TESTS OFF)

# Get all source files from src/ and include/
file(GLOB_RECURSE SOURCES src/*.cpp src/*.h include/*.h)

# Add the include directory for TS_SKSEFunctions
set(TSSKSEFUNCTIONS_INCLUDE_DIR "../TS_SKSEFunctions-main/include")
file(GLOB_RECURSE TSSKSEFUNCTIONS_SOURCES "../TS_SKSEFunctions-main/src/*.cpp" "../TS_SKSEFunctions-main/include/*.h")

# Combine the source files
list(APPEND SOURCES ${TSSKSEFUNCTIONS_SOURCES})

# --- CommonLibSSE: Use local path from environment variable ---
set(COMMONLIBSSE_PATH $ENV{CommonLibSSEPath_NG})

if(NOT COMMONLIBSSE_PATH)
    message(FATAL_ERROR "CommonLibSSEPath_NG environment variable is not set!")
endif()

add_subdirectory("$ENV{CommonLibSSEPath_NG}" CommonLibSSE EXCLUDE_FROM_ALL)

# --- ClibUtils: Use local path from environment variable ---
set(CLIBUTILS_PATH $ENV{CLibUtilsPath})

if(NOT CLIBUTILS_PATH)
    message(FATAL_ERROR "CLibUtilsPath environment variable is not set!")
endif()

add_subdirectory("$ENV{CLibUtilsPath}" ClibUtils EXCLUDE_FROM_ALL)

# Add your SKSE plugin target
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23) # <--- use C++23 standard
target_precompile_headers(${PROJECT_NAME} PRIVATE include/PCH.h) # <--- PCH.h is required!

target_include_directories(${PROJECT_NAME} PRIVATE include ${TSSKSEFUNCTIONS_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/include)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)

# Link to CommonLibSSE (adjust the library name if needed)
target_link_libraries(${PROJECT_NAME} PRIVATE CommonLibSSE::CommonLibSSE)

# Link to CLibUtil (header-only interface library)
target_link_libraries(${PROJECT_NAME} PRIVATE CLibUtil::CLibUtil)

# Link yaml-cpp for YAML timeline format
find_package(yaml-cpp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE yaml-cpp)

# When your SKSE .dll is compiled, this will automatically copy the .dll into your mods folder.
# Only works if you configure DEPLOY_ROOT above (or set the SKYRIM_MODS_FOLDER environment variable)
if(DEFINED OUTPUT_FOLDER)
    # If you specify an <OUTPUT_FOLDER> (including via environment variables)
    # then we'll copy your mod files into Skyrim or a mod manager for you!

    # Copy the SKSE plugin .dll files into the SKSE/Plugins/ folder
    set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")

    message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        VERBATIM
    )

    # If you perform a "Debug" build, also copy .pdb file (for debug symbols)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(
            TARGET "${PROJECT_NAME}"
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
            VERBATIM
        )
    endif()

    # Copy compiled Papyrus scripts (.pex files) from scripts/ to Scripts/ folder
    set(SCRIPTS_FOLDER "${OUTPUT_FOLDER}/Scripts")
    message(STATUS "Papyrus scripts output folder: ${SCRIPTS_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SCRIPTS_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/scripts/FCFW_SKSEFunctions.pex" "${SCRIPTS_FOLDER}/FCFW_SKSEFunctions.pex"
        VERBATIM
    )

    # Copy source Papyrus scripts (.psc files) from source/scripts/ to Source/Scripts/ folder
    set(SOURCE_SCRIPTS_FOLDER "${OUTPUT_FOLDER}/Source/Scripts")
    message(STATUS "Papyrus source scripts output folder: ${SOURCE_SCRIPTS_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SOURCE_SCRIPTS_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/source/scripts/FCFW_SKSEFunctions.psc" "${SOURCE_SCRIPTS_FOLDER}/FCFW_SKSEFunctions.psc"
        VERBATIM
    )

    # Copy Documentation folder and all its contents
    set(DOCUMENTATION_FOLDER "${OUTPUT_FOLDER}/Documentation")
    message(STATUS "Documentation output folder: ${DOCUMENTATION_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Documentation" "${DOCUMENTATION_FOLDER}"
        VERBATIM
    )

    # Copy FCFW_PAPYRUS_REFERENCE.md to Documentation folder
    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Documentation/FCFW_PAPYRUS_REFERENCE.md" "${DOCUMENTATION_FOLDER}/FCFW_PAPYRUS_REFERENCE.md"
        VERBATIM
    )

    # Copy example scripts directly to Documentation output folder (not to project dir)
    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DOCUMENTATION_FOLDER}/PapyrusExample/Scripts"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DOCUMENTATION_FOLDER}/PapyrusExample/Source/Scripts"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/scripts/FCFW_EXAMPLE_QUESTSCRIPT.pex" "${DOCUMENTATION_FOLDER}/PapyrusExample/Scripts/FCFW_EXAMPLE_QUESTSCRIPT.pex"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/source/scripts/FCFW_EXAMPLE_QUESTSCRIPT.psc" "${DOCUMENTATION_FOLDER}/PapyrusExample/Source/Scripts/FCFW_EXAMPLE_QUESTSCRIPT.psc"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/scripts/FCFW_EXAMPLE_PLAYERSCRIPT.pex" "${DOCUMENTATION_FOLDER}/PapyrusExample/Scripts/FCFW_EXAMPLE_PLAYERSCRIPT.pex"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/source/scripts/FCFW_EXAMPLE_PLAYERSCRIPT.psc" "${DOCUMENTATION_FOLDER}/PapyrusExample/Source/Scripts/FCFW_EXAMPLE_PLAYERSCRIPT.psc"
        VERBATIM
    )

    # Copy PapyrusExample to separate example mod folder
    set(EXAMPLE_MOD_FOLDER "C:/Users/tom/AppData/Local/ModOrganizer/MO_SkyrimSE/mods/_DEV_FCFW_EXAMPLE")
    message(STATUS "Example mod output folder: ${EXAMPLE_MOD_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Documentation/PapyrusExample" "${EXAMPLE_MOD_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${EXAMPLE_MOD_FOLDER}/Scripts"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${EXAMPLE_MOD_FOLDER}/Source/Scripts"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/scripts/FCFW_EXAMPLE_QUESTSCRIPT.pex" "${EXAMPLE_MOD_FOLDER}/Scripts/FCFW_EXAMPLE_QUESTSCRIPT.pex"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/source/scripts/FCFW_EXAMPLE_QUESTSCRIPT.psc" "${EXAMPLE_MOD_FOLDER}/Source/Scripts/FCFW_EXAMPLE_QUESTSCRIPT.psc"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/scripts/FCFW_EXAMPLE_PLAYERSCRIPT.pex" "${EXAMPLE_MOD_FOLDER}/Scripts/FCFW_EXAMPLE_PLAYERSCRIPT.pex"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/source/scripts/FCFW_EXAMPLE_PLAYERSCRIPT.psc" "${EXAMPLE_MOD_FOLDER}/Source/Scripts/FCFW_EXAMPLE_PLAYERSCRIPT.psc"
        VERBATIM
    )
endif()
