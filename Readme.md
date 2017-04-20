# TranslucentTB

[![Join on Discord](https://discordapp.com/api/guilds/304387206552879116/widget.png)](https://discord.gg/w95DGTK)  
[![Join the chat at https://gitter.im/TranslucentTB/Lobby](https://badges.gitter.im/TranslucentTB/Lobby.svg)](https://gitter.im/TranslucentTB/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

A lightweight (uses <2MB of RAM and almost no CPU) utility that makes the Windows taskbar translucent/transparent. Sadly only compatible with Windows 10, as TTB uses new Windows 10 APIs.

You can see how to use the command line in the [usage file](usage.md).

You can see examples of the customizations you can make in the image below:
![taskbar images](https://i.imgur.com/QMnfswp.png)

##Add to Startup

To add TranslucentTB to startup (so it opens as soon as you open Windows), first right click `TranslucentTB.exe` and click `Create Shortcut`. Then press <kbd>Win</kbd>+<kbd>R</kbd> and type `shell:startup` click "OK". Then copy your shortcut and paste it in the folder which opens. Voila!

##Features
TranslucentTB supports multiple taskbar states and dynamic taskbar states.

Normal taskbar states (choose one):
 - Blurred, which makes the taskbar, well, blurred.
 - Transparent, which makes the taskbar transparent. This will display your whole desktop background.
 - Opaque, which makes your taskbar opaque.

Dynamic taskbar states (these can be used together):
 - Dynamic Window States (dynamic-ws), which will make the taskbar blurred when a window is maximised on the current monitor and transparent otherwise.
 - Dynamic Start Menu (dynamic-sm), which will make your taskbar match the system theme when the Start Menu is open. Does not change the Start Menu. WIP.

TranslucentTB also supports custom tints, which make your taskbar any color you want.

You can see it in action [here](https://gfycat.com/EverlastingCreamyIlladopsis) (Thanks @Gunny123!).

##Download
You can download the program (prebuilt executables and source snapshots) [via the releases tab](https://github.com/ethanhs/TranslucentTB/releases).

##Security
Some antiviruses are over eager, so they might flag this program as malicious. IT IS NOT! The source is open, you can compile yourself, and I welcome any and all security reviews.

Speaking of compiling...

##Building from source

You have two options here: you can checkout either of the `develop` or `master` branches. It is highly recommended that you checkout `master` as it is stable, and `develop` may contain non-working code.
 Via [git](https://git-scm.com):
```
$ git clone -b master https://github.com/ethanhs/TranslucentTB.git
```

You can also download a zip of each branch by clicking on the `Clone or Download` button.

Now that you have the source, you will need Visual Studio 2015. [You can get the free community edition here](https://www.visualstudio.com/vs/community/). 
Once you have that installed (make sure you have the C++ components, which are included by default). Then you can open TranslucentTB.sln, and press <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>B</kbd> to build the solution.
The output will be in either the Release or Debug folder.

If you don't want to open Visual Studio (or install it), you can (if you haven't installed VS) install the [Visual C++ Build Tools from Microsoft](http://landinghub.visualstudio.com/visual-cpp-build-tools). 
Then open the `Visual C++ MSBuild Command Prompt` and `cd` into the root directory of this project, before running `msbuild`.

##Thanks

I definitly could not have done this without the help of several people:
@charlesmilette, @MrAksel, @olliethepikachu, and last but certainly not least @PFCKrutonium.

If you would like to contribute, everyone is welcome to! If you are considering a major feature, need guidance, 
or want to talk an idea out, don't hesitate to jump on Discord or Gitter(see above), or file an issue. The main contributors are often on Discord, Gitter and Github, so we should reply fairly quickly.
Also, at this time I have no plans of expanding this beyond the taskbar.

###Similar programs
If you are looking for something that modifies more than just the taskbar, there are several programs out there.

[Taskbar Tools](https://github.com/Elestriel/TaskbarTools) is a similar program written in C#. Elestriel plans on expanding beyond the taskbar, to Explorer and Start last I heard.

You may have seen similar translucency abilities from programs such as Start is Back++ and Classic Shell. Both of these are great programs, but I don't need the start-replacement features, so I wrote this.
Also Dynamic States allow for more customisability over the taskbar.

###Known issues

On build 14986, Windows Defender's real time protection sometimes seems to cause Explorer to freeze. There are two work-arounds: temporarily disable Defender when starting it, or start it via the command line.
I do not know if this is fixed in build 15002.

###License

This program is free software under the GPL v3. Please see the COPYING file for more.
