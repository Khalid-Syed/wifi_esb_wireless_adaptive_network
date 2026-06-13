@echo off

set "JFLASH_PATH=C:\Program Files\SEGGER\JLink\JFlash.exe"
set "PROJECT_PATH=nordic.jflash"
set "BINARY_PATH=build\zephyr\merged.hex"
set "LOG_FILE=jflash_log.log"

"%JFLASH_PATH%" -openprj"%PROJECT_PATH%" -open"%BINARY_PATH%" -auto -jflashlog"%LOG_FILE%" -min -exit