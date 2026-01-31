@echo off
echo Starting Particle Simulator with venv and Python 3.12...
echo.

REM Check if venv exists
if not exist venv (
    echo Creating virtual environment with Python 3.12...
    python3.12 -m venv venv
    if errorlevel 1 (
        echo ERROR: Failed to create virtual environment
        pause
        exit /b 1
    )
)

REM Activate venv
echo Activating virtual environment...
call venv\Scripts\activate.bat
if errorlevel 1 (
    echo ERROR: Failed to activate virtual environment
    pause
    exit /b 1
)

REM Install dependencies if needed
pip show PyQt5 >nul 2>&1
if errorlevel 1 (
    echo Installing dependencies...
    pip install -r requirements.txt
)

REM Run the simulator
echo.
echo Starting particle simulator...
python src\python\main.py

deactivate
