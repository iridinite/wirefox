add_library(wirefox-compiler-interface INTERFACE)

if(WARNINGS_PEDANTIC)
  target_compile_options(wirefox-compiler-interface INTERFACE /W4)
endif(WARNINGS_PEDANTIC)
if(WARNINGS_AS_ERRORS)
  target_compile_options(wirefox-compiler-interface INTERFACE /WX)
endif(WARNINGS_AS_ERRORS)

# enable multicore compilation
target_compile_options(wirefox-compiler-interface INTERFACE /MP)
