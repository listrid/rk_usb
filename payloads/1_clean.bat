
SET CROSS_AARCH32_DIR=%~d0\gcc\aarch32\bin\
SET CROSS_AARCH64_DIR=%~d0\gcc\aarch64\bin\


%CROSS_AARCH32_DIR%make.exe clean
if %ERRORLEVEL% NEQ 0  (echo. Error make && pause && exit /b 1 )

rmdir .\bin
