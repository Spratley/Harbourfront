
set OUTPUT_DIR=Generated
set INSTALL_DIR=%CD%\install
set GENERATOR="Visual Studio 17 2022"
set CONFIG_TYPES="Debug;Release"

if not exist %OUTPUT_DIR% mkdir %OUTPUT_DIR%
cd %OUTPUT_DIR%
cmake -G %GENERATOR% -DCMAKE_CONFIGURATION_TYPES=%CONFIG_TYPES% -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% ..\

echo Cleaning up CMake junk.. If you want something done right you've got to do it yourself I guess
cd ..

for /D %%D in (*) do (
	if /I not "%%D"=="%OUTPUT_DIR%" (
		if exist "%%D\cmake_install.cmake" (
			del "%%D\cmake_install.cmake"
		)
		if exist "%%D\CMakeFiles" (
			rmdir /s /q "%%D\CMakeFiles"
		)
	)
)