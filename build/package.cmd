@echo off

echo Creating Panther2K.x64.zip...
7z a -tzip Panther2K.x64.zip .\x64\Release\Leet.Panther2K.SetupUI\* .\config.xml

echo Creating Panther2K.x86.zip...
7z a -tzip Panther2K.x86.zip .\Win32\Release\Leet.Panther2K.SetupUI\* .\config.xml

echo Creating WinParted.x64.zip...
7z a -tzip WinParted.x64.zip .\x64\Release\Leet.WinParted.Bootstrap\*

echo Creating WinParted.x86.zip...
7z a -tzip WinParted.x86.zip .\Win32\Release\Leet.WinParted.Bootstrap\*

echo Done!