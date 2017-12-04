set BOOST_ROOT=..\build\boost\1.64.0\install64
set  LLVM_ROOT=..\build\llvm\4.0.0\package\msvc2015x64

mkdir build
cd build
cmake -G "Visual Studio 15 2017 Win64" .. "-DBOOST_ROOT=%BOOST_ROOT%" "-DLLVM_ROOT=%LLVM_ROOT%" -DCMAKE_INSTALL_PREFIX=../Install

pause
