# ===========================================================================
#
#                            PUBLIC DOMAIN NOTICE
#               National Center for Biotechnology Information
#
#  This software/database is a "United States Government Work" under the
#  terms of the United States Copyright Act.  It was written as part of
#  the author's official duties as a United States Government employee and
#  thus cannot be copyrighted.  This software/database is freely available
#  to the public for use. The National Library of Medicine and the U.S.
#  Government have not placed any restriction on its use or reproduction.
#
#  Although all reasonable efforts have been taken to ensure the accuracy
#  and reliability of the software and data, the NLM and the U.S.
#  Government do not and cannot warrant the performance or results that
#  may be obtained by using this software or data. The NLM and the U.S.
#  Government disclaim all warranties, express or implied, including
#  warranties of performance, merchantability or fitness for any particular
#  purpose.
#
#  Please cite the author in any work or product based on this material.
#
# ===========================================================================

#
# Calculation of the global settings for the CMake build.
#

# allow implicit source file extensions
if ( ${CMAKE_VERSION} VERSION_EQUAL "3.20" OR
     ${CMAKE_VERSION} VERSION_GREATER "3.20")
    cmake_policy(SET CMP0115 OLD)
endif()

set( VERSION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/shared/toolkit.vers")
file( READ ${VERSION_FILE} VERSION )
string( STRIP ${VERSION} VERSION )
message( VERSION=${VERSION} )
string( REGEX MATCH "^[0-9]+" MAJVERS ${VERSION} )

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# determine OS
if ( ${CMAKE_HOST_SYSTEM_NAME} STREQUAL  "Darwin" )
    set(OS "mac")
    set(SHLX "dylib")
elseif ( ${CMAKE_HOST_SYSTEM_NAME} STREQUAL  "Linux" )
    set(OS "linux")
    set(SHLX "so")
elseif ( ${CMAKE_HOST_SYSTEM_NAME} STREQUAL  "Windows" )
    set(OS "windows")
elseif()
    message ( FATAL_ERROR "unknown OS " ${CMAKE_HOST_SYSTEM_NAME})
endif()

# determine architecture
if ( ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "armv7l")
	set(ARCH "armv7l")
elseif ( ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    set(ARCH "aarch64")
elseif ( ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    set(ARCH "x86_64")
elseif ( ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
    set(ARCH "x86_64")
elseif()
    message ( FATAL_ERROR "unknown architecture " ${CMAKE_HOST_SYSTEM_PROCESSOR})
endif ()

# create variables based entirely upon OS
if ( "mac" STREQUAL ${OS} )
    add_compile_definitions( MAC BSD UNIX )
    set( EXE "" )
elseif( "linux" STREQUAL ${OS} )
    add_compile_definitions( LINUX UNIX )
    set( LMCHECK -lmcheck )
    set( EXE "" )
elseif( "windows" STREQUAL ${OS} )
    add_compile_definitions( WINDOWS _WIN32_WINNT=0x0502 )
    set( EXE ".exe" )
endif()

# create variables based entirely upon ARCH
# TODO: check if we need anything besides BITS in the if-else below
if ("armv7l" STREQUAL ${ARCH})
	set( BITS 32 )
	add_compile_options( -Wno-psabi )
elseif ("aarch64" STREQUAL ${ARCH} )
	set ( BITS 64 )
elseif ("x86_64" STREQUAL ${ARCH} )
    set ( BITS 64 )
endif()

# now any unique combinations of OS and ARCH
# TODO: check if this is needed
if     ( "mac-x86_84" STREQUAL ${OS}-${ARCH})
elseif ( "linux-x86_64" STREQUAL ${OS}-${ARCH})
elseif ( "linux-armv7l" STREQUAL ${OS}-${ARCH})
elseif ( "linux-aarch64" STREQUAL ${OS}-${ARCH})
    add_compile_definitions( __float128=_Float128 )
endif()

# combinations of OS and COMP
if ( ${OS}-${CMAKE_CXX_COMPILER_ID} STREQUAL "linux-GNU"  )
    add_definitions( -rdynamic )
    add_compile_definitions( _GNU_SOURCE )
endif()

add_compile_definitions( _ARCH_BITS=${BITS} ${ARCH} ) # TODO ARCH ?
add_definitions( -Wall )

# assume debug build by default
if ( "${CMAKE_BUILD_TYPE}" STREQUAL "" )
    set ( CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE )
endif()

if ( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
    add_compile_definitions( DEBUG _DEBUGGING )
else()
    add_compile_definitions( NDEBUG )
endif()

if ( SINGLE_TARGET )
    message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
endif()

message( "OS=" ${OS} " ARCH=" ${ARCH} " CXX=" ${CMAKE_CXX_COMPILER} " LMCHECK=" ${LMCHECK} " BITS=" ${BITS} " CMAKE_C_COMPILER_ID=" ${CMAKE_C_COMPILER_ID} " CMAKE_CXX_COMPILER_ID=" ${CMAKE_CXX_COMPILER_ID} )

# ===========================================================================
# ncbi-vdb sources

if( NOT VDB_SRCDIR )
    set( VDB_SRCDIR ${CMAKE_SOURCE_DIR}/../ncbi-vdb )
    if ( NOT EXISTS ${VDB_SRCDIR} )
        message( FATAL_ERROR "Please specify the location of ncbi-vdb sources in Cmake variable VDB_SRCDIR")
    endif()
	message( "VDB_SRCDIR: ${VDB_SRCDIR}" )
endif()

include_directories( ${VDB_SRCDIR}/interfaces ) # TODO: introduce a variable pointing to interfaces

if ( "GNU" STREQUAL "${CMAKE_C_COMPILER_ID}")
    include_directories(${VDB_SRCDIR}/interfaces/cc/gcc)
    include_directories(${VDB_SRCDIR}/interfaces/cc/gcc/${ARCH})
elseif ( CMAKE_CXX_COMPILER_ID MATCHES "^(Apple)?Clang$" )
    include_directories(${VDB_SRCDIR}/interfaces/cc/clang)
    include_directories(${VDB_SRCDIR}/interfaces/cc/clang/${ARCH})
elseif ( "MSVC" STREQUAL "${CMAKE_C_COMPILER_ID}")
    include_directories(${VDB_SRCDIR}/interfaces/cc/vc++)
    include_directories(${VDB_SRCDIR}/interfaces/cc/vc++/${ARCH})
endif()

if ( "mac" STREQUAL ${OS} )
    include_directories(${VDB_SRCDIR}/interfaces/os/mac)
    include_directories(${VDB_SRCDIR}/interfaces/os/unix)
elseif( "linux" STREQUAL ${OS} )
    include_directories(${VDB_SRCDIR}/interfaces/os/linux)
    include_directories(${VDB_SRCDIR}/interfaces/os/unix)
elseif( "windows" STREQUAL ${OS} )
    include_directories(${VDB_SRCDIR}/interfaces/os/win)
endif()

include_directories( ${CMAKE_SOURCE_DIR}/ngs/ngs-sdk )

# ===========================================================================
# 3d party packages

# Flex/Bison
find_package( FLEX 2.6 )
find_package( BISON 3 )

find_package( LibXml2 )

find_package(Java COMPONENTS Development)

if ( PYTHON_PATH )
    set( Python3_EXECUTABLE ${PYTHON_PATH} )
endif()
find_package( Python3 COMPONENTS Interpreter )

# ===========================================================================

enable_testing()

# ===========================================================================
# singfle vs. multitarget generators, ncbi-vdb binaries

function(SetAndCreate var path )
    if( NOT DEFINED ${var} )
        set( ${var} ${path} PARENT_SCOPE )
    endif()
    file(MAKE_DIRECTORY ${path} )
endfunction()

if ( ${CMAKE_GENERATOR} MATCHES "Visual Studio.*" OR
     ${CMAKE_GENERATOR} STREQUAL "Xcode" )
    set( SINGLE_CONFIG false )

    if( NOT VDB_BINDIR OR NOT EXISTS ${VDB_BINDIR} )
        message( FATAL_ERROR "Please specify the location of an ncbi-vdb build in Cmake variable VDB_BINDIR. It is expected to contain one or both subdirectories Debug/ and Release/, with bin/, lib/ and ilib/ underneath each.")
    endif()

    set( NCBI_VDB_BINDIR_DEBUG ${VDB_BINDIR}/Debug/bin )
    set( NCBI_VDB_BINDIR_RELEASE ${VDB_BINDIR}/Release/bin )

    set( NCBI_VDB_LIBDIR_DEBUG ${VDB_BINDIR}/Debug/lib )
    set( NCBI_VDB_LIBDIR_RELEASE ${VDB_BINDIR}/Release/lib )

    set( NCBI_VDB_ILIBDIR_DEBUG ${VDB_BINDIR}/Debug/ilib )
    set( NCBI_VDB_ILIBDIR_RELEASE ${VDB_BINDIR}/Release/ilib )

    SetAndCreate( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/Debug/bin )
    SetAndCreate( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release/bin )
    SetAndCreate( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/Debug/lib )
    SetAndCreate( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release/lib )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG            ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE          ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE} )
    set( CMAKE_JAR_OUTPUT_DIRECTORY                      ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG} ) # both Debug and Release share this

    # to be used in add-test() as the location of executables.
    # NOTE: always use the COMMAND_EXPAND_LISTS option of add_test
    set( BINDIR "$<$<CONFIG:Debug>:${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}>$<$<CONFIG:Release>:${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}>" )

    link_directories( $<$<CONFIG:Debug>:${NCBI_VDB_LIBDIR_DEBUG}> $<$<CONFIG:Release>:${NCBI_VDB_LIBDIR_RELEASE}> )
    link_directories( $<$<CONFIG:Debug>:${NCBI_VDB_ILIBDIR_DEBUG}> $<$<CONFIG:Release>:${NCBI_VDB_ILIBDIR_RELEASE}> )

else() # assume a single-config generator
    set( SINGLE_CONFIG true )

    if( NOT VDB_BINDIR OR NOT EXISTS ${VDB_BINDIR} )
        message( FATAL_ERROR "Please specify the location of an ncbi-vdb build in Cmake variable VDB_BINDIR. It is expected to contain subdirectories bin/, lib/, ilib/.")
    endif()

    set( NCBI_VDB_BINDIR ${VDB_BINDIR}/bin )
    set( NCBI_VDB_LIBDIR ${VDB_BINDIR}/lib )
    set( NCBI_VDB_ILIBDIR ${VDB_BINDIR}/ilib )

    SetAndCreate( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
    SetAndCreate( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY          ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} )
    set( CMAKE_JAR_OUTPUT_DIRECTORY              ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} )

    # to be used in add-test() as the location of executables.
    # NOTE: always use the COMMAND_EXPAND_LISTS option of add_test
    set( BINDIR "${CMAKE_BINARY_DIR}/bin" )

    link_directories( ${NCBI_VDB_LIBDIR} )
    link_directories( ${NCBI_VDB_ILIBDIR} )
endif()

function( GenerateStaticLibsWithDefs target_name sources compile_defs )
    add_library( ${target_name} STATIC ${sources} )
    if( NOT "" STREQUAL "${compile_defs}" )
        target_compile_definitions( ${target_name} PRIVATE ${compile_defs} )
    endif()
endfunction()

function( GenerateStaticLibs target_name sources )
    GenerateStaticLibsWithDefs( ${target_name} "${sources}" "" )
endfunction()

function( ExportStatic name install )
    # the output goes to .../lib
    if( SINGLE_CONFIG )
        # make the output name versioned, create all symlinks
        set_target_properties( ${name} PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
        add_custom_command(TARGET ${name}
            POST_BUILD
            COMMAND rm -f lib${name}.a.${VERSION}
            COMMAND mv lib${name}.a lib${name}.a.${VERSION}
            COMMAND ln -f -s lib${name}.a.${VERSION} lib${name}.a.${MAJVERS}
            COMMAND ln -f -s lib${name}.a.${MAJVERS} lib${name}.a
            COMMAND ln -f -s lib${name}.a lib${name}-static.a
            WORKING_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )
        if ( ${install} )
            install( FILES  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib${name}.a.${VERSION}
                            ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib${name}.a.${MAJVERS}
                            ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib${name}.a
                            ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib${name}-static.a
                    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib64
            )
         endif()
    else()
        set_target_properties( ${name} PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG})
        set_target_properties( ${name} PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE})
    endif()
endfunction()


#
# create versioned names and symlinks for a shared library
#
function(MakeLinksShared target name install)
    if( SINGLE_CONFIG )
        add_custom_command(TARGET ${target}
            POST_BUILD
            COMMAND rm -f lib${name}.${SHLX}.${VERSION}
            COMMAND mv lib${name}.${SHLX} lib${name}.${SHLX}.${VERSION}
            COMMAND ln -f -s lib${name}.${SHLX}.${VERSION} lib${name}.${SHLX}.${MAJVERS}
            COMMAND ln -f -s lib${name}.${SHLX}.${MAJVERS} lib${name}.${SHLX}
            WORKING_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )
        if ( ${install} )
            install( FILES  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib${name}.${SHLX}.${VERSION}
                            ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib${name}.${SHLX}.${MAJVERS}
                            ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib${name}.${SHLX}
                    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib64
        )
        endif()
    endif()
endfunction()

#
# for a static library target, create a public shared target with the same base name and contents
#
function(ExportShared lib install)
    if ( NOT WIN32 )
        get_target_property( src ${lib} SOURCES )
        add_library( ${lib}-shared SHARED ${src} )
        set_target_properties( ${lib}-shared PROPERTIES OUTPUT_NAME ${lib} )
        MakeLinksShared( ${lib}-shared ${lib} ${install} )
    endif()
endfunction()

set( COMMON_LINK_LIBRARIES kapp tk-version )

if( WIN32 )
    set( CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" )
    set( COMMON_LINK_LIBRARIES  ${COMMON_LINK_LIBRARIES} Ws2_32 Crypt32 )
    # unset(CMAKE_IMPORT_LIBRARY_SUFFIX) # do not generate import libraries
    # set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  /INCREMENTAL:NO" )
endif()
