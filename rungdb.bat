
@echo Sorry, KeeperRL crashed :(
@echo.
@echo Gathering crash information and sending crash home a report.
@echo.
@echo It may take a few minutes. Please don't close this window.

@echo off


gdb.exe --data-directory gdb -batch -p %1 -x gdb_input.txt >> stacktrace.out
sendreport.bat