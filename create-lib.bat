cd build\optimized
lib /machine:x64 /def:libmonetdb5.def

cd ..\debug
lib /machine:x64 /def:libmonetdb5.def

cd ..\..
