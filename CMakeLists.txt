#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

if (DEFINED AZURE_MQTT_BUILT)
    RETURN()
endif()

project(umqtt)

#making a nice global variable to know if we are on linux or not.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LINUX TRUE)
    #on Linux, enable valgrind
    #these commands (MEMORYCHECK...) need to apear BEFORE include(CTest) or they will not have any effect
    find_program(MEMORYCHECK_COMMAND valgrind)	
    set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --error-exitcode=1 --track-origins=yes")
endif()

include(CheckSymbolExists)
function(detect_architecture symbol arch) 
    if (NOT DEFINED ARCHITECTURE) 
        set(CMAKE_REQUIRED_QUIET 1)
        check_symbol_exists("${symbol}" "" ARCHITECTURE_${arch}) 
        unset(CMAKE_REQUIRED_QUIET) 
  
        # The output variable needs to be unique across invocations otherwise 
        # CMake's crazy scope rules will keep it defined 
        if (ARCHITECTURE_${arch}) 
            set(ARCHITECTURE "${arch}" PARENT_SCOPE) 
            set(ARCHITECTURE_${arch} 1 PARENT_SCOPE) 
            add_definitions(-DARCHITECTURE_${arch}=1) 
        endif() 
    endif() 
endfunction() 

if (MSVC) 
    detect_architecture("_M_AMD64" x86_64) 
    detect_architecture("_M_IX86" x86) 
    detect_architecture("_M_ARM" ARM) 
else() 
    detect_architecture("__x86_64__" x86_64) 
    detect_architecture("__i386__" x86) 
    detect_architecture("__arm__" ARM) 
endif() 
if (NOT DEFINED ARCHITECTURE) 
    set(ARCHITECTURE "GENERIC") 
endif() 
message(STATUS "Target architecture: ${ARCHITECTURE}") 

include (CTest)

include(ExternalProject)

#the following variables are project-wide and can be used with cmake-gui
option(skip_unittests "set skip_unittests to ON to skip unittests (default is OFF)[if possible, they are always build]" OFF)
option(compileOption_C "passes a string to the command line of the C compiler" OFF)
option(compileOption_CXX "passes a string to the command line of the C++ compiler" OFF)

#Use solution folders. 
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Start of variables used during install
#set (INCLUDE_INSTALL_DIR include CACHE PATH "Include file directory")

add_subdirectory(azure-c-shared-utility)

enable_testing()

#if any compiler has a command line switch called "OFF" then it will need special care
if(NOT "${compileOption_C}" STREQUAL "OFF")
    set(CMAKE_C_FLAGS "${compileOption_C} ${CMAKE_C_FLAGS}")
endif()

if(NOT "${compileOption_CXX}" STREQUAL "OFF")
    set(CMAKE_CXX_FLAGS "${compileOption_CXX} ${CMAKE_CXX_FLAGS}")
endif()

#this project uses several other projects that are build not by these CMakeFiles
#this project also targets several OSes

#this function takes care of three things:
#1. copying some shared libraries(.dll or .so) to the location of the output executable

macro(compileAsC99)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_C_FLAGS "--std=c99 ${CMAKE_C_FLAGS}")
      set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
    endif()
  else()
    set (CMAKE_C_STANDARD 99)
    set (CMAKE_CXX_STANDARD 11)
  endif()
endmacro(compileAsC99)

macro(compileAsC11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_C_FLAGS "--std=c11 ${CMAKE_C_FLAGS}")
      set (CMAKE_C_FLAGS "-D_POSIX_C_SOURCE=200112L ${CMAKE_C_FLAGS}")
      set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
    endif()
  else()
    set (CMAKE_C_STANDARD 11)
    set (CMAKE_CXX_STANDARD 11)
  endif()
endmacro(compileAsC11)

#Update_dependencies()

compileAsC11()

#these are the C source files
set(source_c_files
./src/mqtt_client.c
./src/mqtt_codec.c
./src/mqtt_message.c
)

#these are the C headers
set(source_h_files
./inc/azure_umqtt_c/mqtt_client.h
./inc/azure_umqtt_c/mqtt_codec.h
./inc/azure_umqtt_c/mqttconst.h
./inc/azure_umqtt_c/mqtt_message.h
)

#the following "set" statetement exports across the project a global variable called COMMON_INC_FOLDER that expands to whatever needs to included when using COMMON library
set(MQTT_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using sharedLib lib" FORCE)
set(MQTT_SRC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/src CACHE INTERNAL "this is what needs to be included when doing include sources" FORCE)
include_directories(${MQTT_INC_FOLDER} ${SHARED_UTIL_INC_FOLDER})

get_directory_property(hasParent PARENT_DIRECTORY)
if(hasParent)
    set(AZURE_MQTT_BUILT "1C283849-1933-4197-B9AC" PARENT_SCOPE)
endif()

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

#this is the product (a library)
add_library(umqtt ${source_c_files} ${source_h_files})

set(SHARED_UTIL_ADAPTER_FOLDER "${CMAKE_CURRENT_LIST_DIR}/azure-c-shared-utility/c/adapters")
if(WIN32)
    set(LOCK_C_FILE ${SHARED_UTIL_ADAPTER_FOLDER}/lock_c11.c)
    set(THREAD_C_FILE ${SHARED_UTIL_ADAPTER_FOLDER}/threadapi_c11.c)
else()
    set(LOCK_C_FILE ${SHARED_UTIL_ADAPTER_FOLDER}/lock_pthreads.c)
    set(THREAD_C_FILE ${SHARED_UTIL_ADAPTER_FOLDER}/threadapi_pthreads.c)
endif()

target_link_libraries(umqtt aziotsharedutil)

if (NOT ${ARCHITECTURE} STREQUAL "ARM")
	add_subdirectory(samples)
endif()

if (NOT ${skip_unittests})
    add_subdirectory(tests)
endif()
