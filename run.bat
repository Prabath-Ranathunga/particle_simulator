@echo off
REM Run Script for Windows
REM Check if virtual environment exists
if not exist venv (
    echo ERROR: Virtual environment not found!
    echo Please run setup_windows.bat first
    pause
    exit /b 1
)
REM Check if C++ module exists
if not exist src\python\particle_renderer.pyd (
    echo ERROR: C++ renderer module not found!
    echo Please run setup_windows.bat to build the project
    pause
    exit /b 1
)
REM Activate virtual environment
call venv\Scripts\activate.bat
REM Run application
echo ========================================
echo   Starting ...
echo ========================================
echo.
python src\python\main.py
REM If it exits with error, pause to see error message
if errorlevel 1 (
    echo.
    echo Application exited with an error
    pause
)