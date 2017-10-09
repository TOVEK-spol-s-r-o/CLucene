%~d0
cd "%~p0"

copy /Y build64\bin\debug\clucene-contribs-libd.* \Build\x64\winD\
copy /Y build64\bin\release\clucene-contribs-lib.* \Build\x64\winR\
copy /Y build64\bin\debug\clucene-cored.* \Build\x64\winD\
copy /Y build64\bin\release\clucene-core.* \Build\x64\winR\
copy /Y build64\bin\debug\clucene-sharedd.* \Build\x64\winD\
copy /Y build64\bin\release\clucene-shared.* \Build\x64\winR\

mkdir \devlibs2012\clucene_2_3_2_tvk_4\
mkdir \devlibs2012\clucene_2_3_2_tvk_4\lib
mkdir \devlibs2012\clucene_2_3_2_tvk_4\lib\x64

copy /Y build64\bin\debug\clucene-contribs-libd.* \devlibs2012\clucene_2_3_2_tvk_4\lib\x64
copy /Y build64\bin\release\clucene-contribs-lib.* \devlibs2012\clucene_2_3_2_tvk_4\lib\x64
copy /Y build64\bin\debug\clucene-cored.* \devlibs2012\clucene_2_3_2_tvk_4\lib\x64
copy /Y build64\bin\release\clucene-core.* \devlibs2012\clucene_2_3_2_tvk_4\lib\x64
copy /Y build64\bin\debug\clucene-sharedd.* \devlibs2012\clucene_2_3_2_tvk_4\lib\x64
copy /Y build64\bin\release\clucene-shared.* \devlibs2012\clucene_2_3_2_tvk_4\lib\x64

copy /Y build64\bin\debug\clucene-contribs-libd.* \\data\Development\devlibs\libs2012\clucene_2_3_2_tvk_4\lib\x64
copy /Y build64\bin\release\clucene-contribs-lib.* \\data\Development\devlibs\libs2012\clucene_2_3_2_tvk_4\lib\x64
copy /Y build64\bin\debug\clucene-cored.* \\data\Development\devlibs\libs2012\clucene_2_3_2_tvk_4\lib\x64
copy /Y build64\bin\release\clucene-core.* \\data\Development\devlibs\libs2012\clucene_2_3_2_tvk_4\lib\x64
copy /Y build64\bin\debug\clucene-sharedd.* \\data\Development\devlibs\libs2012\clucene_2_3_2_tvk_4\lib\x64
copy /Y build64\bin\release\clucene-shared.* \\data\Development\devlibs\libs2012\clucene_2_3_2_tvk_4\lib\x64



@echo off

pause
