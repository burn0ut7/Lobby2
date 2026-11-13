@echo off
set "GC_WORKBENCH_INSTALL_SCRIPT=%~dp0support\Install.ps1"
start "" "%~dp0support\payload\helper\GC Tools.exe" --tools
