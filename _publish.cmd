%~d0
cd "%~p0"

copy /Y build\bin\debug\clucene-contribs-libd.* \Build\win32\winD\
copy /Y build\bin\release\clucene-contribs-lib.* \Build\win32\winR\
copy /Y build\bin\debug\clucene-cored.* \Build\win32\winD\
copy /Y build\bin\release\clucene-core.* \Build\win32\winR\
copy /Y build\bin\debug\clucene-sharedd.* \Build\win32\winD\
copy /Y build\bin\release\clucene-shared.* \Build\win32\winR\
@echo off

PATH=%PATH%;"c:\Program Files (x86)\Microsoft Visual Studio 8\Common7\Tools\Bin\"

echo Patching manifest of clucene-contribs-lib.dll 
mt.exe -manifest d:\DevLibs\manifest\msVC80.manifest -outputresource:d:\Build\win32\winR\clucene-contribs-lib.dll;2

echo Patching manifest of clucene-core.dll 
mt.exe -manifest d:\DevLibs\manifest\msVC80.manifest -outputresource:d:\Build\win32\winR\clucene-core.dll;2

echo Patching manifest of clucene-shared.dll 
mt.exe -manifest d:\DevLibs\manifest\msVC80.manifest -outputresource:d:\Build\win32\winR\clucene-shared.dll;2

pause
