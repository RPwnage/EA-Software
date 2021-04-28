set CHROMEPATH="%ProgramFiles(x86)%\Google\Chrome\Application\chrome.exe"

cd %~dp0

start "" %CHROMEPATH% --new-window --allow-file-access-from-files  --user-data-dir=%TMP% file://"%~dp0webui.html"