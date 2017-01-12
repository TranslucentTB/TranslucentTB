# TranslucentTB

[![Join the chat at https://gitter.im/TranslucentTB/Lobby](https://badges.gitter.im/TranslucentTB/Lobby.svg)](https://gitter.im/TranslucentTB/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

A lightweight (uses <2MB of ram and almost no cpu) utility that makes the Windows taskbar translucent/transparent. Sadly only compatible with Windows 10.

You can see how to use the command line in the [usage file](usage.md)

You can see examples of the customizations you can make in the image below:
![taskbar images](https://i.imgur.com/QMnfswp.png)


##Download
You can download the program (prebuilt binaries and source snapshots) [via the releases tab](https://github.com/ethanhs/TranslucentTB/releases).

##Security
Some antiviruses are over eager, so they might flag this program as malicious. IT IS NOT! The source is open, you can compile yourself, and I welcome any and all security reviews.

Speaking of compiling...

##Building from source

You have two options here: you can checkout either of the `develop` or `master` branches. It is highly recommended that you checkout master as it is stable, and develop may contain non-working code.
 Via git:
```
$ git clone -b master https://github.com/ethanhs/TranslucentTB.git
```

You can also download a zip of each branch by clicking on the `Clone or Download` button.

Now that you have the source, you will need Visual Studio 2015. [You can get the free community edition here](https://www.visualstudio.com/vs/community/). 
Once you have that installed (make sure you have the C++ components, which are included by default). Then you can open TranslucentTB.sln, and press <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>B</kbd> to build the solution.
The output will be in either the Release or Debug folder.

##Thanks

I definitly could not have done this without the help of several people:
@charlesmilette, @MrAksel, @olliethepikachu, and last but certainly not least @PFCKrutonium.

If you would like to contribute, everyone is welcome to! If you are considering a major feature, need guidance, 
or want to talk an idea out, don't hesitate to jump on Gitter (see above), or file an issue. I am often on Gitter and Github, so I should reply fairly quickly.
Also, at this time I have no plans of expanding this beyond the taskbar.

###Similar programs
If you are looking for something that modifies more than just the taskbar, there are several programs out there.

[Taskbar Tools](https://github.com/Elestriel/TaskbarTools) is a similar program written in C#. Elestriel plans on expanding beyond the taskbar, to Explorer and Start last I heard.

You may have seen similar abilities from programs such as Start is Back++ and Classic Shell. Both of these are great programs, but I don't need the start-replacement features, so I wrote this.

###Known issues

On build 14986, Windows defenders real time protection sometimes seems to cause Explorer to freeze. There are two work-arounds: temporarily disable defender when starting it, or start it via the command line.
I do not know if this is fixed in build 15002.