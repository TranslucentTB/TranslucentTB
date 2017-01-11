# TranslucentTB

[![Join the chat at https://gitter.im/TranslucentTB/Lobby](https://badges.gitter.im/TranslucentTB/Lobby.svg)](https://gitter.im/TranslucentTB/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
A lightweight utility that makes the Windows taskbar translucent/transparent.

#Download
You can download the program [via the releases tab](https://github.com/ethanhs/TranslucentTB/releases).

#Compile
Want to compile yourself? My releases were compiled using the Microsoft Visual Studio build tools. You'll need Visual Studio first.

Open "Visual C++ 2015 x86 Native Build Tools Command Prompt" from the Start menu. Once you've `cd`'d into the code's directory, `cd` into the TranslucentTB folder and run `cl main.cpp /Fe"TranslucentTb" /link /subsystem:windows user32.lib` to create `TranslucentTb.exe`.