:: Delete UE4 temporary
rmdir /s /q "Binaries"
rmdir /s /q "Intermediate"
rmdir /s /q "Saved"

:: Delete VS temporary
rmdir /s /q ".vs"
del /q "ChangingGuns.sln"
