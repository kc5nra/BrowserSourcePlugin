
pushd %_CWD% 
set _CWD=%CD% 
popd 

for /f "delims=" %%a in ('"%GIT_BIN_DIR%\git" describe') do @set BSP_VERSION_BARE=%%a

set BSP_VERSION="BSP_VERSION=L"%BSP_VERSION_BARE%-x86""
msbuild /t:Rebuild /property:Configuration=Release;Platform=Win32

copy Plugin\Release\BrowserSourcePlugin.dll Release\BrowserSourcePlugin
copy Wrapper\Release\BrowserSourcePluginWrapper.dll Release\

cd Release
7z a BSP-%BSP_VERSION_BARE%-x86.7z .
move BSP-%BSP_VERSION_BARE%-x86.7z ..
cd ..
