@echo off

set "app_name=Fuku"

setlocal

cd %~dp0

if not exist build mkdir build
cd build

if "%Platform%" neq "x64" (
	echo ERROR: Platform is not "x64" - please run this from the MSVC x64 native tools command prompt.
	goto end
)

set "warn_options=-Wall -Wextra -Wshadow -Wno-missing-prototypes -Wno-missing-variable-declarations -Wno-declaration-after-statement -Wno-unused-parameter -Wno-extra-semi-stmt -Wno-cast-qual -Wno-unsafe-buffer-usage"

set "compile_options=%warn_options% /nologo /DAPP_NAME=\"%app_name%\" /I../vendor"
set "link_options=/incremental:no /opt:ref /subsystem:windows user32.lib gdi32.lib opengl32.lib"

if "%1"=="debug" (
	set "compile_options=%compile_options% /Od /Z7 /Zo /RTC1"
	set "link_options=%link_options% /DEBUG:FULL libucrtd.lib libvcruntimed.lib"
) else if "%1"=="release" (
	set "compile_options=%compile_options% /O2"
	set "link_options=%link_options% libvcruntime.lib"
) else (
	goto invalid_arguments
)

set "build_platform=false"
set "build_game=false"

if "%2"=="all" (
	set "build_platform=true"
	set "build_game=true"
) else if "%2"=="platform" (
	set "build_platform=true"
) else if "%2"=="game" (
	set "build_game=true"
) else (
	goto invalid_arguments
)

if "%3" neq  "" goto invalid_arguments

if "%build_platform%"=="true" (
	clang-cl %compile_options% ../src/platform_win32.c /link %link_options% /pdb:"%app_name%.pdb" /out:"%app_name%.exe"
)

if "%build_game%"=="true" (
	REM
)

goto end

:invalid_arguments
echo Invalid Arguments^. Usage: build ^[debug ^| release^] ^[all ^| platform ^| game^]
goto end

:end
endlocal
