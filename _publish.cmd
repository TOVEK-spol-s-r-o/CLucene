%~d0
cd "%~p0"

copy /Y build\bin\debug\clucene-contribs-libd.* \Build\win32\winD\
copy /Y build\bin\release\clucene-contribs-lib.* \Build\win32\winR\
copy /Y build\bin\debug\clucene-cored.* \Build\win32\winD\
copy /Y build\bin\release\clucene-core.* \Build\win32\winR\
copy /Y build\bin\debug\clucene-sharedd.* \Build\win32\winD\
copy /Y build\bin\release\clucene-shared.* \Build\win32\winR\

mkdir \devlibs2012\clucene_2_3_2_tvk_7\
mkdir \devlibs2012\clucene_2_3_2_tvk_7\lib
mkdir \devlibs2012\clucene_2_3_2_tvk_7\lib\win32
mkdir \\data\Development\devlibs\libs2012\clucene_2_3_2_tvk_7
mkdir \\data\Development\devlibs\libs2012\clucene_2_3_2_tvk_7\lib
mkdir \\data\Development\devlibs\libs2012\clucene_2_3_2_tvk_7\lib\win32

copy /Y build\bin\debug\clucene-contribs-libd.* \devlibs2012\clucene_2_3_2_tvk_7\lib\win32
copy /Y build\bin\release\clucene-contribs-lib.* \devlibs2012\clucene_2_3_2_tvk_7\lib\win32
copy /Y build\bin\debug\clucene-cored.* \devlibs2012\clucene_2_3_2_tvk_7\lib\win32
copy /Y build\bin\release\clucene-core.* \devlibs2012\clucene_2_3_2_tvk_7\lib\win32
copy /Y build\bin\debug\clucene-sharedd.* \devlibs2012\clucene_2_3_2_tvk_7\lib\win32
copy /Y build\bin\release\clucene-shared.* \devlibs2012\clucene_2_3_2_tvk_7\lib\win32

copy /Y build\bin\debug\clucene-contribs-libd.* \\data\Development\devlibs\libs2012\clucene_2_3_2_tvk_7\lib\win32
copy /Y build\bin\release\clucene-contribs-lib.* \\data\Development\devlibs\libs2012\clucene_2_3_2_tvk_7\lib\win32
copy /Y build\bin\debug\clucene-cored.* \\data\Development\devlibs\libs2012\clucene_2_3_2_tvk_7\lib\win32
copy /Y build\bin\release\clucene-core.* \\data\Development\devlibs\libs2012\clucene_2_3_2_tvk_7\lib\win32
copy /Y build\bin\debug\clucene-sharedd.* \\data\Development\devlibs\libs2012\clucene_2_3_2_tvk_7\lib\win32
copy /Y build\bin\release\clucene-shared.* \\data\Development\devlibs\libs2012\clucene_2_3_2_tvk_7\lib\win32

@echo off

pause
