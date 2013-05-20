%~d0
cd "%~p0"

copy /Y build64\bin\debug\clucene-contribs-libd.* \Build\win64\winD\
copy /Y build64\bin\release\clucene-contribs-lib.* \Build\win64\winR\
copy /Y build64\bin\debug\clucene-cored.* \Build\win64\winD\
copy /Y build64\bin\release\clucene-core.* \Build\win64\winR\
copy /Y build64\bin\debug\clucene-sharedd.* \Build\win64\winD\
copy /Y build64\bin\release\clucene-shared.* \Build\win64\winR\
@echo off

PATH=%PATH%;"c:\Program Files (x86)\Microsoft Visual Studio 8\Common7\Tools\Bin\"

echo Patching manifest of clucene-contribs-lib.dll 
mt.exe -manifest d:\DevLibs\manifest\msVC80_amd64.manifest -outputresource:d:\Build\win64\winR\clucene-contribs-lib.dll;2

echo Patching manifest of clucene-core.dll 
mt.exe -manifest d:\DevLibs\manifest\msVC80_amd64.manifest -outputresource:d:\Build\win64\winR\clucene-core.dll;2

echo Patching manifest of clucene-shared.dll 
mt.exe -manifest d:\DevLibs\manifest\msVC80_amd64.manifest -outputresource:d:\Build\win64\winR\clucene-shared.dll;2

pause
