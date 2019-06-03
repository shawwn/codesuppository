@echo on

pushd "%~dp0"

set XPJ="xpj4.exe"

%XPJ% -v 4 -t vc14 -p win32 -x Pd3d.xpj || goto pauseExit
%XPJ% -v 4 -t vc14 -p win32 -x CodeSuppository.xpj || goto pauseExit
REM %XPJ% -v 4 -t vc14 -p win64 -x CodeSuppository.xpj || goto pauseExit

goto cleanExit

:pauseExit
pause

:cleanExit
popd

