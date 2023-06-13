

project(PythonBindings)

set (Sources 
		Bindings.cpp
	)
	
set (Headers

	)
	

source_group("" FILES ${Sources} ${Headers})


pybind11_add_module(PythonBindings ${Sources})
target_include_directories(PythonBindings PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(${PROJECT_NAME} PUBLIC CompoundFs)


#target_link_libraries(${PROJECT_NAME} PUBLIC CompoundFs)
