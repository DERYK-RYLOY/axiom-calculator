#!/bin/sh
# Works on macOS, Linux, and Android (Termux)
echo "Building Advanced Calculator (C++17)..."
CXX=${CXX:-g++}
$CXX -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp -o calc
if [ $? -ne 0 ]; then
  echo "BUILD FAILED - install g++ or clang (Termux: pkg install clang)."
  echo "You can still use calculator.html in any browser."
  exit 1
fi
echo "Build OK. Run ./calc to start."
