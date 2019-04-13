:: Delete UE4 temporary
rmdir /s /q "Binaries"
rmdir /s /q "Intermediate"

:: Delete VS temporary
rmdir /s /q ".vs"
del /q *.sln
