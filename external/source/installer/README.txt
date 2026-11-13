GC WORKBENCH TOOLS - LOBBY2 - WINDOWS X64

Install from Workbench: Plugins > GC > GC Tools > Install external dependencies.
Save and restart Workbench after installation. Subsequent normal launches load
Lobby2's dropdown DLL automatically through a profile bootstrap. No special
shortcut, MCP installation, .NET installation or developer tools are required.
The bootstrap loads only its own checksum-verified DLL into its launching
Workbench. It exposes no generic editor commands or suppression features.

Requires supported Workbench 1.9.0.37 and its exact Qt 6.8.3 binaries.
Enable NET API in Workbench preferences and save preferences before first
installation. GC adapters use the configured port on loopback; the editor may
listen on other interfaces. This installer never changes those preferences.
Lobby2 supplies the GC choice providers and mission bridge. The DLL applies
to gc-choices fields; other editor fields keep normal behavior.

Installed files live under %LOCALAPPDATA%/GCWorkbench. Changed bootstrap or
Git helper files are preserved and reported. The installed state directory
records each process's native startup outcome and report. An uncertain load
is never replayed in the same process; save and restart Workbench to retry.
Normal Workbench script authorization may ask permission to run the bootstrap.
Authorization policy and window suppression belong to separate developer tools.

Git for Windows is installed quietly from its official GitHub release, with
release checksum and Windows signature verification. GC Tools uses Git Credential
Manager's browser sign-in; it does not collect GitHub passwords.

Uninstall: save and close Workbench, extract the ZIP and run Uninstall.bat.
Only GC-owned startup/helpers and installation files are removed. Other tools,
editor settings, projects and Git are preserved. No live DLL is forcibly unloaded.

Sources and packaging belong to Lobby2/external/source. Development/test addons
under external/development are not distributed in this Windows package or the
Workshop release. Reforger MCP and the website are separate products and installs.
