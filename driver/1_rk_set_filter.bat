@echo off
net sess>nul 2>&1||(powershell saps '%0'-Verb RunAs&exit)
cd /d %~dp0
.\amd64\install-filter.exe install "--device=USB\VID_2207&PID_350A\ROCKCHIP"
.\amd64\install-filter.exe install "--device=USB\VID_2207&PID_350F\ROCKCHIP"
.\amd64\install-filter.exe install "--device=USB\VID_2207&PID_110C\ROCKCHIP"
pause