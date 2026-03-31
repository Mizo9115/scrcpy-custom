@echo off
set SCRCPY_DEFAULT_ARGS=--turn-screen-off --screen-off-key --stay-awake --no-audio
scrcpy.exe --pause-on-exit=if-error %SCRCPY_DEFAULT_ARGS% %*
