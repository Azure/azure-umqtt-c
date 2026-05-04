#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

usePermissiveRulesForSamplesAndTests()

if (TARGET c_logging_v2)
    link_libraries(c_logging_v2)
endif()

#this is CMakeLists.txt for the folder tests of mqtt
add_subdirectory(mqtt_client_ut)
add_subdirectory(mqtt_codec_ut)
add_subdirectory(mqtt_message_ut)

