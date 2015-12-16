#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for lock_unittests
cmake_minimum_required(VERSION 3.0)

compileAsC11()
set(theseTestsName mqtt_message_unittests)

set(${theseTestsName}_cpp_files
${theseTestsName}.cpp
)

set(${theseTestsName}_c_files
../../src/mqtt_message.c
)

set(${theseTestsName}_h_files
)

include_directories(${MQTT_SRC_FOLDER})

build_test_artifacts(${theseTestsName} ON)
