#[[###########################################################################

        Theoride Driver Project

    © 2017 Theoride Technology (http://theolizer.com/) All Rights Reserved.

    General Public License Version 3 ("GPLv3")
        You may use this file in accordance with the terms and conditions of 
        GPLv3 published by Free Software Foundation.
        Please confirm the contents of GPLv3 at https://www.gnu.org/licenses/gpl.txt .
        A copy of GPLv3 is also saved in a LICENSE.TXT file.

    General Public License Version 3(以下GPLv3)
        Free Software Foundationが公表するGPLv3の使用条件に従って、
        あなたはこのファイルを取り扱うことができます。
        GPLv3の内容を https://www.gnu.org/licenses/gpl.txt にて確認して下さい。
        またGPLv3のコピーをLICENSE.TXTファイルにおいてます。

]]############################################################################

cmake_minimum_required(VERSION 3.5.0)

#-----------------------------------------------------------------------------
#       Project version
#-----------------------------------------------------------------------------

project(Driver VERSION 0.0.1)

message(STATUS "PROJECT_VERSION=${PROJECT_VERSION}")

file(WRITE ${CMAKE_BINARY_DIR}/compiler.txt "${CMAKE_CXX_COMPILER}")

#-----------------------------------------------------------------------------
#       default compile options
#-----------------------------------------------------------------------------

# for MSVC
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configs" FORCE)
set(CMAKE_SUPPRESS_REGENERATION TRUE)

if (MSVC)
    add_definitions(-D_UNICODE -DUNICODE)
    string(REPLACE "/W3 " "/W4 " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj -D_MSC_VER=${MSVC_VERSION}")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
endif()

#-----------------------------------------------------------------------------
#       Boost
#-----------------------------------------------------------------------------

message(STATUS "BOOST_ROOT=${BOOST_ROOT}")
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

find_package(Boost 1.54.0 REQUIRED COMPONENTS filesystem system)
message(STATUS "  Boost_INCLUDE_DIR=${Boost_INCLUDE_DIR}")
message(STATUS "  Boost_LIBRARY_DIR=${Boost_LIBRARY_DIR}")
message(STATUS "  Boost_LIBRARIES  =${Boost_LIBRARIES}")

#-----------------------------------------------------------
#       setup LLVM(libTooling)
#-----------------------------------------------------------

message(STATUS "LLVM_ROOT : ${LLVM_ROOT}")

if("${LLVM_ROOT}" STREQUAL "")
    find_package(LLVM ${LLVM_VERSION} REQUIRED CONFIG)
else()
    find_package(LLVM ${LLVM_VERSION} REQUIRED CONFIG
        PATHS "${LLVM_ROOT}/share/llvm/cmake" "${LLVM_ROOT}/lib/cmake/llvm")
endif()
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

if(LLVM_BUILD_MAIN_SRC_DIR)
  include_directories(${LLVM_BUILD_MAIN_SRC_DIR}/tools/clang/include)
  include_directories(${LLVM_BUILD_BINARY_DIR}/tools/clang/include)
endif()
add_definitions(${LLVM_DEFINITIONS})

set(LIB_TOOLING_LIST                 clangFrontend clangSerialization clangDriver)
set(LIB_TOOLING_LIST ${LIB_TOOLING_LIST} clangTooling clangParse clangSema)
set(LIB_TOOLING_LIST ${LIB_TOOLING_LIST} clangAnalysis clangEdit clangAST)
set(LIB_TOOLING_LIST ${LIB_TOOLING_LIST} clangLex clangBasic clangRewrite)
set(LIB_TOOLING_LIST ${LIB_TOOLING_LIST} LLVMProfileData LLVMObject LLVMMCParser LLVMBitReader)
set(LIB_TOOLING_LIST ${LIB_TOOLING_LIST} LLVMCore LLVMMC LLVMOption LLVMSupport)

if(WIN32)
    set(WINLIB_LIST Version)
endif()

#-----------------------------------------------------------------------------
#       Install settings
#-----------------------------------------------------------------------------

# install destinations
set(BIN_DIR bin)

#-----------------------------------------------------------------------------
#       target definition
#-----------------------------------------------------------------------------

set(THEORIDE_DRIVER_LIST driver.cpp avoid-trouble.h header_includes.h utility.h parse.h)

message(STATUS "Sources of TheorideDriver")
foreach(SOURCE IN LISTS THEORIDE_DRIVER_LIST)
    message(STATUS "${SOURCE}")
endforeach()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/bin/Debug")

#-----------------------------------------------------------
#       make target
#-----------------------------------------------------------

#       ---<<< Common >>>---

# Header
include_directories("${Boost_INCLUDE_DIR}")
include_directories("${LLVM_INCLUDE_DIRS}")

# Library
link_directories("${Boost_LIBRARY_DIR}")
link_directories("${LLVM_LIBRARY_DIRS}")

#       ---<<< TheorideDriver >>>---

add_executable(TheorideDriver ${THEORIDE_DRIVER_LIST})
#set_target_properties(TheorideDriver PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD_RELEASE FALSE)

target_link_libraries(TheorideDriver ${Boost_LIBRARIES})

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-comment")
else()
    add_custom_command(TARGET TheorideDriver
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:TheorideDriver> $<TARGET_FILE_DIR:TheorideDriver>/cl.exe)
endif()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    foreach(link_lib IN LISTS LIB_TOOLING_LIST)
        target_link_libraries(TheorideDriver optimized ${link_lib}.lib)
        target_link_libraries(TheorideDriver debug     ${link_lib}d.lib)
    endforeach()
else()
    target_link_libraries(TheorideDriver ${LIBRARY_LIST} ${LIB_TOOLING_LIST})
    if (CMAKE_COMPILER_IS_MINGW)
        set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
    endif()
endif()
if(NOT "${WINLIB_LIST}" STREQUAL "")
    target_link_libraries(TheorideDriver ${WINLIB_LIST}.lib)
endif()

llvm_map_components_to_libnames(LLVM_LIBS ${COMPONENT_LIST})
target_link_libraries(TheorideDriver ${LLVM_LIBS})

message(STATUS "Boost_LIBRARIES  = ${Boost_LIBRARIES}")
message(STATUS "LIBRARY_LIST     = ${LIBRARY_LIST}")
message(STATUS "LIB_TOOLING_LIST = ${LIB_TOOLING_LIST}")
message(STATUS "WINLIB_LIST      = ${WINLIB_LIST}")

message(STATUS "COMPONENT_LIST   = ${COMPONENT_LIST}")
message(STATUS "LLVM_LIBRARY_DIRS= ${LLVM_LIBRARY_DIRS}")
message(STATUS "LLVM_LIBS        = ${LLVM_LIBS}")

# 以下デバッグ用
get_property(LIBRARIES TARGET TheorideDriver PROPERTY LINK_LIBRARIES)
message(STATUS "LIBRARIES = ${LIBRARIES}")
message(STATUS "CMAKE_CXX_IMPLICIT_LINK_LIBRARIES = ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES}")
message(STATUS "CMAKE_C_FLAGS = ${CMAKE_C_FLAGS}")
message(STATUS "CMAKE_C_FLAGS_RELEASE = ${CMAKE_C_FLAGS_RELEASE}")
message(STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE = ${CMAKE_CXX_FLAGS_RELEASE}")

#-----------------------------------------------------------
#       install target
#-----------------------------------------------------------

install(PROGRAMS $<TARGET_FILE:TheorideDriver> CONFIGURATIONS Release DESTINATION ${BIN_DIR})

if (MSVC)
    install(FILES $<TARGET_FILE:TheorideDriver>
            CONFIGURATIONS Release
            DESTINATION bin
            RENAME cl.exe)
endif()

#-----------------------------------------------------------------------------
#       テスト登録
#-----------------------------------------------------------------------------

enable_testing()

set(ORIGINAL_COMPILER "${CMAKE_CXX_COMPILER}")
set(PARAMETERS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
string(REPLACE " " ";" PARAMETERS "${PARAMETERS}")

add_test(NAME TheorideDriver
    COMMAND $<TARGET_FILE:TheorideDriver>
        ""
        -Dtheoride_original_compiler=${ORIGINAL_COMPILER}
        ${PARAMETERS}
        test.cpp
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test"
)
