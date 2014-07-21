del TM.exe
del error.txt
set LIBRARY_PATH=C:\Program Files\Microsoft Speech SDK 5.1\Lib\i386
set CPLUS_INCLUDE_PATH=C:\Program Files\Microsoft Speech SDK 5.1\Include
C:\mingw-w64\i686-4.9.0-win32-dwarf-rt_v3-rev2\mingw32\bin\g++ -static src/*.cpp -I include -O3 -Wall libmusicxml3.dll -lcomdlg32 -lole32 -lsapi -lshlwapi -lwinmm -s -o TM.exe 2>error.txt
