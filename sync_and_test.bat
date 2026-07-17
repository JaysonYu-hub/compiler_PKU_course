@echo off
set TEST_DIR=%~1
if "%TEST_DIR%"=="" set TEST_DIR=debug/2026/functional
set MODE=%~2
if "%MODE%"=="" set MODE=64

echo =============================================
echo Step 1/3: Syncing local code to RISC-V VM...
echo =============================================
wsl rsync -rlvzc --delete -e "ssh -p 2222 -i /home/cs61c/.ssh/id_rsa -o StrictHostKeyChecking=no" --exclude="build" --exclude=".git" --exclude="debug" /mnt/d/project/compiler_src/compiler/ ubuntu@localhost:/home/ubuntu/compiler/

if %errorlevel% neq 0 (
    echo Sync failed! Please check if the VM is running.
    exit /b %errorlevel%
)

echo.
echo =============================================
echo Step 2/3: Compiling compiler inside VM...
echo =============================================
wsl ssh -p 2222 ubuntu@localhost -i /home/cs61c/.ssh/id_rsa -o StrictHostKeyChecking=no "cd compiler && cmake -B build -S . && cmake --build build && cp build/compiler ."

if %errorlevel% neq 0 (
    echo Compile inside VM failed!
    exit /b %errorlevel%
)

echo.
echo =============================================
echo Step 3/3: Running test program in VM...
echo =============================================
wsl ssh -p 2222 ubuntu@localhost -i /home/cs61c/.ssh/id_rsa -o StrictHostKeyChecking=no "cd compiler && python3 vm_autotest_sy.py %TEST_DIR% --mode %MODE%"
