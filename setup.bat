@echo off
REM Complete Setup Script for Windows
echo ========================================
echo   Setup
echo ========================================
echo.
REM Check Python installation
echo [1/5] Checking Python...
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found!
    pause
    exit /b 1
)
python --version
REM Create virtual environment
echo.
echo [2/5] Creating Python virtual environment...
if exist venv (
    echo Virtual environment already exists, skipping...
) else (
    python -m venv venv
    if errorlevel 1 (
        echo ERROR: Failed to create virtual environment
        pause
        exit /b 1
    )
)
REM Activate virtual environment
echo.
echo [3/5] Activating virtual environment...
call venv\Scripts\activate.bat
if errorlevel 1 (
    echo ERROR: Failed to activate virtual environment
    pause
    exit /b 1
)
REM Upgrade pip and install dependencies
echo.
echo [4/5] Installing Python dependencies...
python -m pip install --upgrade pip --quiet
pip install -r requirements.txt
if errorlevel 1 (
    echo ERROR: Failed to install Python dependencies
    pause
    exit /b 1
)
echo.
echo ========================================
echo   Building C++ Renderer
echo ========================================
echo.
echo [5/5] Compiling C++ code...
echo.
REM Check if CMake exists
cmake --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found!
    echo.
    echo Please install one of the following:
    echo   1. Visual Studio 2019/2022 with "C++ CMake tools"
    echo   2. CMake from https://cmake.org/download/
    echo.
    pause
    exit /b 1
)
REM Create build directory
if not exist build mkdir build
cd build
REM Configure with CMake
echo Configuring build system...
echo.
REM Try different CMake configurations
REM First try vcpkg
cmake .. -G "Visual Studio 17 2022" -A x64 >nul 2>&1
if errorlevel 1 (
    REM Try Visual Studio 2019
    cmake .. -G "Visual Studio 16 2019" -A x64 >nul 2>&1
    if errorlevel 1 (
        REM Try MinGW
        cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release >nul 2>&1
        if errorlevel 1 (
            echo ERROR: CMake configuration failed!
            cd ..
            pause
            exit /b 1
        )
        set BUILD_SYSTEM=MinGW
    ) else (
        set BUILD_SYSTEM=VS2019
    )
) else (
    set BUILD_SYSTEM=VS2022
)
echo Using build system: %BUILD_SYSTEM%
echo.

REM Build
echo Building... (this may take a minute)
cmake --build . --config Release
if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    echo.
    cd ..
    pause
    exit /b 1
)

REM Copy output
echo.
echo Copying module to Python directory...
if exist Release\particle_renderer.pyd (
    copy Release\particle_renderer.pyd ..\src\python\ >nul
) else if exist particle_renderer.pyd (
    copy particle_renderer.pyd ..\src\python\ >nul
)

cd ..

REM Test import
echo.
echo Testing module import...
python -c "import particle_renderer; print('SUCCESS: Module loaded')" 2>nul
if errorlevel 1 (
    echo WARNING: Module import failed
    echo This might be due to missing DLLs (e.g., glew32.dll)
    echo.
    echo Try copying GLEW DLL to src\python\ directory
) else (
    echo SUCCESS: Module working correctly!
)

echo.
echo ========================================
echo   Setup Complete!
echo ========================================
echo.
echo To run :
echo   Double-click run.bat
echo.
pause