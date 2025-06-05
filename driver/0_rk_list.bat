
@echo off
net sess>nul 2>&1||(powershell saps '%0'-Verb RunAs&exit)
cd /d %~dp0

.\amd64\install-filter.exe list --class="Rockusb Device"
pause