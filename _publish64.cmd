%~d0
cd "%~p0"

copy /Y build64\bin\debug\clucene-contribs-libd.* \Build\x64\winD\
copy /Y build64\bin\release\clucene-contribs-lib.* \Build\x64\winR\
copy /Y build64\bin\debug\clucene-cored.* \Build\x64\winD\
copy /Y build64\bin\release\clucene-core.* \Build\x64\winR\
copy /Y build64\bin\debug\clucene-sharedd.* \Build\x64\winD\
copy /Y build64\bin\release\clucene-shared.* \Build\x64\winR\

pause
