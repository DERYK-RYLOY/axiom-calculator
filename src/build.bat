@echo off
echo Building Advanced Calculator (C++17)...
g++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp -o calc.exe
if errorlevel 1 (
  echo.
  echo BUILD FAILED - make sure g++ is installed and in PATH.
  echo You can still use calculator.html in any browser.
  exit /b 1
)
echo.
echo Build OK. Run calc.exe to start, or open calculator.html for the touch version.
