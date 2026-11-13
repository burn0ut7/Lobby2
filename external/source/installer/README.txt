GC WORKBENCH TOOLS - GC LOBBY - WINDOWS X64

Install from Workbench: Plugins > GC > GC Tools > Install external dependencies.
Save and restart Workbench after installation. Subsequent normal launches load
GC Lobby's dropdown DLL automatically through a profile bootstrap. No special
shortcut, MCP installation, .NET installation or developer tools are required.
The bootstrap loads only its own checksum-verified DLL into its launching
Workbench. It exposes no generic editor commands or suppression features.

Requires supported Workbench 1.9.0.37 and its exact Qt 6.8.3 binaries.
Enable NET API in Workbench preferences and save preferences before first
installation. GC adapters use the configured port on loopback; the editor may
listen on other interfaces. This installer never changes those preferences.
GC Lobby supplies the GC dropdown and string lookup providers. The DLL applies
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

Uninstall: use Dependencies > Uninstall in GC Tools, or run Uninstall.bat.
Save and restart Workbench normally afterward. Old version files are cleaned
up after Workbench and GC Tools close; a later reinstall is preserved.
Package caches and diagnostic logs remain available.
Both dependencies are selected by default. Install/Update and Uninstall apply
only to the selected items. Selecting Git for uninstall runs its registered
Windows uninstaller. Uninstall.bat removes only the GC integration.
Editor settings and projects are preserved. No live DLL is forcibly unloaded.

Sources and packaging belong to Lobby2/external/source. Development/test addons
under external/development are not distributed in this Windows package or the
Workshop release. Reforger MCP and the website are separate products and installs.
