@echo off
rem Builds the phone package from the command line (Qt Creator does the same through its
rem Symbian kit). Qt for Symbian supports in-source builds only, so this runs in the
rem project directory; the generated files are listed in .gitignore.
rem
rem   build-symbian.cmd            release ARMv5 build + self-signed SimpleOKM.sis
rem   build-symbian.cmd installer  also wraps it in the Smart Installer package
rem                                (SimpleOKM_installer.sis) for Symbian Anna phones
rem   build-symbian.cmd clean      removes the build output
setlocal
call D:\QtSDK\Symbian\SDKs\SymbianSR1Qt474\env.bat
cd /d %~dp0

if "%1"=="clean" (
    call sbs -c arm.v5.urel.gcce4_4_1 clean
    goto :eof
)

qmake SimpleOKM.pro -spec symbian-sbsv2 CONFIG+=release
if errorlevel 1 exit /b 1
call sbs -c arm.v5.urel.gcce4_4_1
if errorlevel 1 exit /b 1

if "%1"=="installer" (
    call createpackage.bat -i SimpleOKM_installer.pkg release-armv5
) else (
    call createpackage.bat SimpleOKM_template.pkg release-armv5
)
