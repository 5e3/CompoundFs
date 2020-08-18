

project(Sandbox)

set (Sources 
		main.cpp
	)
	
set (Headers

	)
	

source_group("" FILES ${Sources} ${Headers})

add_executable(${PROJECT_NAME} ${Sources} ${Headers})
target_link_libraries(${PROJECT_NAME} PUBLIC CompoundFs)
