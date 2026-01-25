@echo off
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

REM ================== 配置区 ==================
set HEX_FILE=stk500boot_v2_mega2560.hex
set BIN_FILE=boot.bin
set OUT_HEADER=bootloader_image.h
set TMP_HEADER=%OUT_HEADER%.tmp

set ARRAY_NAME=boot_bin
REM ============================================

REM ============ 参数处理 ============
if /I "%1"=="clean" goto CLEAN
if "%1"=="" goto BUILD

echo [ERROR] Unknown parameter: %1
echo Usage:
echo   %~nx0        ^(build^)
echo   %~nx0 clean  ^(remove generated files^)
exit /b 1


REM ============ CLEAN ============
:CLEAN
echo [CLEAN] Removing generated files...
if exist "%BIN_FILE%" del /f /q "%BIN_FILE%"
if exist "%OUT_HEADER%" del /f /q "%OUT_HEADER%"
if exist "%TMP_HEADER%" del /f /q "%TMP_HEADER%"
echo [CLEAN] Done.
exit /b 0


REM ============ BUILD ============
:BUILD
echo [BUILD] Convert HEX -> BIN
if not exist "%HEX_FILE%" (
    echo [ERROR] HEX file not found: %HEX_FILE%
    exit /b 1
)

avr-objcopy -I ihex "%HEX_FILE%" -O binary "%BIN_FILE%"
if errorlevel 1 (
    echo [ERROR] avr-objcopy failed
    exit /b 1
)

echo [BUILD] Convert BIN -> C header (xxd)
xxd -i "%BIN_FILE%" > "%TMP_HEADER%"
if errorlevel 1 (
    echo [ERROR] xxd failed
    exit /b 1
)

echo [BUILD] Patch header: name=%ARRAY_NAME%, type=const uint8_t, add PROGMEM

powershell -NoProfile -ExecutionPolicy Bypass ^
  "$tmp = '%TMP_HEADER%';" ^
  "$out = '%OUT_HEADER%';" ^
  "$target = '%ARRAY_NAME%';" ^
  "$c = Get-Content -Raw $tmp;" ^
  "if ($c -notmatch 'unsigned char\s+([A-Za-z0-9_]+)\[\]\s*=\s*\{') { throw 'Cannot find xxd array declaration'; }" ^
  "$src = $Matches[1];" ^
  "$c = $c -replace ('unsigned char\s+' + [regex]::Escape($src) + '\[\]\s*=\s*\{'), ('const uint8_t ' + $target + '[] PROGMEM = {');" ^
  "$c = $c -replace ('unsigned int\s+' + [regex]::Escape($src) + '_len\s*=\s*'), ('const unsigned int ' + $target + '_len = ');" ^
  "Set-Content -NoNewline -Encoding ASCII $out $c;"

if errorlevel 1 (
    echo [ERROR] powershell patch failed
    exit /b 1
)

del /f /q "%TMP_HEADER%" >nul 2>&1

echo [SUCCESS] Create %OUT_HEADER% with: const uint8_t %ARRAY_NAME%[] PROGMEM
exit /b 0
