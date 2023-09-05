

project(py_txfs)

set (Sources 
		Bindings.cpp
	)
	
set (Headers

	)
	

source_group("" FILES ${Sources} ${Headers})


pybind11_add_module(py_txfs ${Sources})
target_include_directories(py_txfs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(${PROJECT_NAME} PUBLIC CompoundFs)


