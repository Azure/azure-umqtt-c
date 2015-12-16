#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for samples. There's noithig here, except redirections to 
#individual protocol samples

function(add_sample_directory whatIsBuilding)
	add_subdirectory(${whatIsBuilding})

	set_target_properties(${whatIsBuilding}
			   PROPERTIES
			   FOLDER "Samples")
endfunction()

add_sample_directory(mqtt_client_sample)

