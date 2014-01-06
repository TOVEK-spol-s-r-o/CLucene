%~d0
cd "%~p0"

copy /Y build\bin\debug\clucene-contribs-libd.* \Build\win32\winD\
copy /Y build\bin\release\clucene-contribs-lib.* \Build\win32\winR\
copy /Y build\bin\debug\clucene-cored.* \Build\win32\winD\
copy /Y build\bin\release\clucene-core.* \Build\win32\winR\
copy /Y build\bin\debug\clucene-sharedd.* \Build\win32\winD\
copy /Y build\bin\release\clucene-shared.* \Build\win32\winR\
@echo off

pause
