@echo off
if not exist "%appdata%\rich_edit_scroll.ini" (
    copy rich_edit_scroll.ini "%appdata%
)
reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /V RichEditScroll /T REG_SZ /F /D "\"%~dp0Release\rich_edit_scroll.exe\""
