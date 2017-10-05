#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(${use_installed_dependencies})
    find_package(azure_c_shared_utility REQUIRED CONFIG)
else()
    if (${original_run_e2e_tests} OR ${original_run_unittests})
        add_subdirectory(deps/azure-c-testrunnerswitcher)
        add_subdirectory(deps/azure-ctest)
        add_subdirectory(deps/umock-c)
    endif()
    add_subdirectory(deps/c-utility)
endif()