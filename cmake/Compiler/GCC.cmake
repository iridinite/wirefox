add_library(wirefox-compiler-interface INTERFACE)

# GCC
if(WARNINGS_PEDANTIC)
  target_compile_options(wirefox-compiler-interface INTERFACE -Wall -Wextra -pedantic)
endif(WARNINGS_PEDANTIC)
if(WARNINGS_AS_ERRORS)
  target_compile_options(wirefox-compiler-interface INTERFACE -Werror)
endif(WARNINGS_AS_ERRORS)
