@echo off
@setlocal enabledelayedexpansion

SET USE_DEBUG=1
SET USE_HOT_RELOAD=1
@REM SET USE_OPTIMIZATIONS=1

@REM Advapi is used for winreg which accesses the windows registry
@REM to get cpu clock frequency which is used with rdtsc.
@REM I have not found an easier way to get the frequency.

if !USE_OPTIMIZATIONS!==1 (
    SET MSVC_COMPILE_OPTIONS=/std:c++14 /nologo /TP /EHsc /O2
) else (
    SET MSVC_COMPILE_OPTIONS=/std:c++14 /nologo /TP /EHsc
)

SET MSVC_LINK_OPTIONS=/NOLOGO /INCREMENTAL:NO /ignore:4099 Advapi32.lib gdi32.lib shell32.lib user32.lib OpenGL32.lib
SET MSVC_INCLUDE_DIRS=/Iinclude /Ilibs/stb/include /Ilibs/glfw-3.3.8/include /Ilibs/glew-2.1.0/include /Ilibs/glm/include
SET MSVC_DEFINITIONS=/DOS_WINDOWS /FI pch.h

if !USE_HOT_RELOAD!==1 (
    SET MSVC_DEFINITIONS=!MSVC_DEFINITIONS! /DUSE_HOT_RELOAD

    tasklist /fi "IMAGENAME eq program.exe" | find /I "program.exe" && if %errorlevel% == 0 (
        echo Game is on!
        SET ONLY_HOT_RELOAD=1
    )
    SET MSVC_LINK_OPTIONS=!MSVC_LINK_OPTIONS! libs/glfw-3.3.8/lib/glfw3dll.lib libs/glew-2.1.0/lib/glew32.lib
    SET MSVC_DEFINITIONS=!MSVC_DEFINITIONS! /DGLFW_DLL
    @REM These may say Sharing violation
    echo f | XCOPY /y /q libs\glfw-3.3.8\lib\glfw3.dll bin\glfw3.dll > nul
    echo f | XCOPY /y /q libs\glew-2.1.0\lib\glew32.dll bin\glew32.dll > nul
) else (
    SET MSVC_LINK_OPTIONS=!MSVC_LINK_OPTIONS! libs/glfw-3.3.8/lib/glfw3_mt.lib libs/glew-2.1.0/lib/glew32s.lib
    SET MSVC_DEFINITIONS=!MSVC_DEFINITIONS! /DGLEW_STATIC
)

if !USE_DEBUG!==1 (
    SET MSVC_COMPILE_OPTIONS=!MSVC_COMPILE_OPTIONS! /Zi
    SET MSVC_LINK_OPTIONS=!MSVC_LINK_OPTIONS! /DEBUG /PROFILE
)

mkdir bin 2> nul

SET srcfile=bin\all.cpp
SET srcfiles=
SET output=bin\program.exe

type nul > !srcfile!
for /r %%i in (*.cpp) do (
    SET file=%%i
    if "x!file:__=!"=="x!file!" if "x!file:bin=!"=="x!file!" (
        if not "x!file:rtssim=!"=="x!file!" (
            echo #include ^"!file:\=/!^">> !srcfile!
        ) else if not "x!file:Engone=!"=="x!file!" (
            echo #include ^"!file:\=/!^">> !srcfile!
        )
    )
)

set /a startTime=6000*( 100%time:~3,2% %% 100 ) + 100* ( 100%time:~6,2% %% 100 ) + ( 100%time:~9,2% %% 100 )

SET compileSuccess=0

cl /c !MSVC_COMPILE_OPTIONS! !MSVC_INCLUDE_DIRS! !MSVC_DEFINITIONS! !srcfile! /Fobin/all.obj
if !ONLY_HOT_RELOAD!==1 (
    @REM SET compileSuccess=!errorlevel!
    link bin\all.obj !MSVC_LINK_OPTIONS! /DLL /OUT:bin/game.dll /PDBALTPATH:hotloaded.pdb
    @REM SET compileSuccess=!errorlevel!
) else (
    link bin\all.obj !MSVC_LINK_OPTIONS! /OUT:!output!
    SET compileSuccess=!errorlevel!

    link bin\all.obj !MSVC_LINK_OPTIONS! /DLL /OUT:bin/game.dll
    @REM SET compileSuccess=!errorlevel!
)

set /a endTime=6000*(100%time:~3,2% %% 100 )+100*(100%time:~6,2% %% 100 )+(100%time:~9,2% %% 100 )
set /a finS=(endTime-startTime)/100
set /a finS2=(endTime-startTime)%%100

echo Compiled in %finS%.%finS2% seconds

if !compileSuccess! == 0 if not !ONLY_HOT_RELOAD!==1 (
    @REM echo f | XCOPY /y /q !output! prog.exe > nul
    if !USE_HOT_RELOAD!==1 (
        start bin\program.exe
    ) else (
        bin\program.exe
    )
)