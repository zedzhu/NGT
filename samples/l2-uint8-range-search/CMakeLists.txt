if( ${UNIX} )
	include_directories("${PROJECT_BINARY_DIR}/lib")
        include_directories("${PROJECT_SOURCE_DIR}/lib")
        link_directories("${PROJECT_SOURCE_DIR}/lib/NGT")

	add_executable(l2-uint8-range-search l2-uint8-range-search.cpp)
	add_dependencies(l2-uint8-range-search ngt)
	target_link_libraries(l2-uint8-range-search ngt pthread)
endif()
