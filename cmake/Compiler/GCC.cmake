# GCC
if(WARNINGS_PEDANTIC)
  target_compile_options(${LIBRARY_NAME} PRIVATE -Wall -Wextra -pedantic)
endif(WARNINGS_PEDANTIC)
if(WARNINGS_AS_ERRORS)
  target_compile_options(${LIBRARY_NAME} PRIVATE -Werror)
endif(WARNINGS_AS_ERRORS)
