
#!/bin/bash
# SPI FLASH RZT1 R4 flashing.
JFLASH_PATH="/Applications/SEGGER/JLink_V758d/JFlash.app/Contents/MacOS/JFlashExe"
PROJECT_PATH="jflash-5340.jflash"
BINARY_PATH="build_wifi/zephyr/merged.hex"
LOG_FILE="jflash_log.log"
"${JFLASH_PATH}" -openprj"${PROJECT_PATH}" -open"${BINARY_PATH}" -auto -jflashlog${LOG_FILE} -min -exit