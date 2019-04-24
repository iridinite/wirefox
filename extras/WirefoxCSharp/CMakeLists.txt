# --- preconditions ---

if(NOT BUILD_C_BINDINGS)
	message(WARNING "C# bindings library requires C bindings to work, but BUILD_C_BINDINGS is set to OFF. If bindings are missing, C# bindings library will crash on startup.")
endif()
if(NOT BUILD_SHARED_LIBS)
	message(FATAL_ERROR "Building C# projects requires BUILD_SHARED_LIBS, because C# assemblies cannot statically link to C++ code.")
endif()

# --- project setup ---

enable_language(CSharp)
set(LIBRARY_NAME "WirefoxSharp")
include(CSharpUtilities)

set(CMAKE_CSharp_FLAGS "/langversion:7 /platform:anycpu")

add_library(${LIBRARY_NAME} SHARED
    Channel.cs
    NativeMethods.cs
    Peer.cs
    PeerID.cs
    Packet.cs
    Enumerations.cs
    JetBrains.Annotations.cs
    Properties/AssemblyInfo.cs
)

set_property(TARGET ${LIBRARY_NAME} PROPERTY VS_DOTNET_REFERENCES
    "System"
    "System.Core"
)

set_property(TARGET ${LIBRARY_NAME} PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7")
set_property(TARGET ${LIBRARY_NAME} PROPERTY VS_GLOBAL_ROOTNAMESPACE "Iridinite.Wirefox")
set_property(TARGET ${LIBRARY_NAME} PROPERTY OUTPUT_NAME "WirefoxSharp")

copy_wirefox_library()
