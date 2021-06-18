@echo welcome ameba1
robocopy "..\..\Arduino_package\release" "%LOCALAPPDATA%\Arduino15\staging\packages"  /e
copy "..\..\Arduino_package\package_realtek.com_ameba1_early_index.json" "%LOCALAPPDATA%\Arduino15"
copy "..\..\Arduino_package\package_realtek.com_ameba1_index.json" "%LOCALAPPDATA%\Arduino15"
copy "..\..\Arduino_package\package_realtek.com_ameba1_raw_index.json" "%LOCALAPPDATA%\Arduino15"
