set CODE_DIR=..\..\code-gen
set DATA_DIR=..\..\game-data-gen
set LUBAN_DLL=.\Tools\Luban\Luban.dll
set CONF_ROOT=.
dotnet %LUBAN_DLL% ^
    -t server ^
    -c cpp-rawptr-bin ^
    -d bin ^
    --conf %CONF_ROOT%\luban.conf ^
    -x outputCodeDir=%CODE_DIR%\cfg-gen\ ^
    -x outputDataDir=%DATA_DIR%\bytes ^
    -x lubanGoModule=luban 
pause