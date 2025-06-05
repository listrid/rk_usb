
SET CROSS_AARCH32_DIR=%~d0\gcc\aarch32\bin\
SET CROSS_AARCH64_DIR=%~d0\gcc\aarch64\bin\

%CROSS_AARCH32_DIR%make.exe all
if %ERRORLEVEL% NEQ 0  (echo. Error make && pause && exit /b 1 )

cd dump-arm32

%CROSS_AARCH32_DIR%make.exe all
if %ERRORLEVEL% NEQ 0  (echo. Error make && pause && exit /b 1 )

cd ../dump-arm64

%CROSS_AARCH32_DIR%make.exe all
if %ERRORLEVEL% NEQ 0  (echo. Error make && pause && exit /b 1 )

cd ../
%CROSS_AARCH32_DIR%make.exe rm_o

mkdir .\bin
move .\dump-arm32\dump-arm32.bin .\bin\dump-arm32.bin
move .\dump-arm64\dump-arm64.bin .\bin\dump-arm64.bin
move .\exec-arm32.bin .\bin\exec-arm32.bin
move .\exec-arm64.bin .\bin\exec-arm64.bin
move .\write-arm32.bin .\bin\write-arm32.bin
move .\write-arm64.bin .\bin\write-arm64.bin
