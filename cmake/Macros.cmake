macro(copy_wirefox_library)
if(BUILD_SHARED_LIBS)
	add_custom_command(TARGET ${LIBRARY_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"$<TARGET_FILE:Wirefox>"
			"$<TARGET_FILE_DIR:${LIBRARY_NAME}>")
endif()
endmacro()
