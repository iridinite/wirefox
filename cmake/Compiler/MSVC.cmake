if(WARNINGS_PEDANTIC)
  message(STATUS "MSVC: ${LIBRARY_NAME}: enabled warning level 4")
  target_compile_options(${LIBRARY_NAME} PRIVATE /W4)
endif(WARNINGS_PEDANTIC)
if(WARNINGS_AS_ERRORS)
  message(STATUS "MSVC: ${LIBRARY_NAME}: enabled warnings-as-errors")
  target_compile_options(${LIBRARY_NAME} PRIVATE /WX)
endif(WARNINGS_AS_ERRORS)

# enable multicore compilation
message(STATUS "MSVC: ${LIBRARY_NAME}: enabled multi-core compilation")
target_compile_options(${LIBRARY_NAME} PRIVATE /MP)
