%~d0
cd "%~p0"

copy /Y build64\bin\debug\clucene-contribs-libd.* \Build\win64\winD\
copy /Y build64\bin\release\clucene-contribs-lib.* \Build\win64\winR\
copy /Y build64\bin\debug\clucene-cored.* \Build\win64\winD\
copy /Y build64\bin\release\clucene-core.* \Build\win64\winR\
copy /Y build64\bin\debug\clucene-sharedd.* \Build\win64\winD\
copy /Y build64\bin\release\clucene-shared.* \Build\win64\winR\

pause
