add_library(wirefox-platform-interface INTERFACE)

target_compile_definitions(wirefox-platform-interface
  INTERFACE
    -D_WIN32_WINNT=0x0601
	-DWIN32_LEAN_AND_MEAN
	-DNOMINMAX)
