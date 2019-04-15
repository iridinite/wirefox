enable_language(CSharp)
set(LIBRARY_NAME "DemoBattleships")
include(CSharpUtilities)

set(CMAKE_CSharp_FLAGS "/langversion:7")

add_executable(${LIBRARY_NAME}
  App.config
  Program.cs
  Board.cs
  BoardPosition.cs
  GameCommand.cs
  Client.cs
  Server.cs
  Ship.cs
  ShipType.cs
  ShotResult.cs

  FormConnect.cs
  FormConnect.Designer.cs
  FormConnect.resx
  FormGame.cs
  FormGame.Designer.cs
  FormGame.resx

  Properties/AssemblyInfo.cs
  Properties/Resources.Designer.cs
  Properties/Resources.resx
  Properties/Settings.Designer.cs
  Properties/Settings.settings
)

target_link_libraries(${LIBRARY_NAME} PRIVATE WirefoxCSharp)

csharp_set_windows_forms_properties(
  FormConnect.cs
  FormConnect.Designer.cs
  FormConnect.resx
  FormGame.cs
  FormGame.Designer.cs
  FormGame.resx
)

csharp_set_designer_cs_properties(
  Properties/AssemblyInfo.cs
  Properties/Resources.Designer.cs
  Properties/Resources.resx
  Properties/Settings.Designer.cs
  Properties/Settings.settings
)

set_property(TARGET ${LIBRARY_NAME} PROPERTY VS_DOTNET_REFERENCES
  "System"
  "System.Core"
  "System.Drawing"
  "System.Windows.Forms"
)

set_property(TARGET ${LIBRARY_NAME} PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7")
set_property(TARGET ${LIBRARY_NAME} PROPERTY VS_GLOBAL_ROOTNAMESPACE "Iridinite.Wirefox.Battleships")
set_property(TARGET ${LIBRARY_NAME} PROPERTY WIN32_EXECUTABLE TRUE)

copy_wirefox_library()
