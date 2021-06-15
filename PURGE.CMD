@echo off
echo Purging Jobs Tree ...
del *.err
del /sqxyz d:\jobmon\files\*.*
md d:\jobmon\files
echo Done.
