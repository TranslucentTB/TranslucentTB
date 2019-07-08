# TranslucentTB

[![Build status](https://ci.appveyor.com/api/projects/status/9yym3vr6s5gc7vk3/branch/master?svg=true)](https://ci.appveyor.com/project/sylveon/translucenttb/branch/master)
[![CodeFactor](https://www.codefactor.io/repository/github/translucenttb/translucenttb/badge/master)](https://www.codefactor.io/repository/github/translucenttb/translucenttb/overview/master)
[![Join on Discord](https://img.shields.io/discord/304387206552879116.svg)](https://discord.gg/w95DGTK)
[![Join on Gitter](https://badges.gitter.im/TranslucentTB/Lobby.svg)](https://gitter.im/TranslucentTB/Lobby)
[![Total downloads](https://img.shields.io/github/downloads/TranslucentTB/TranslucentTB/total.svg)](https://github.com/TranslucentTB/TranslucentTB/releases)
[![Liberapay patrons](https://img.shields.io/liberapay/patrons/TranslucentTB.svg)](https://liberapay.com/TranslucentTB/)

A lightweight (uses a few MB of RAM and almost no CPU) utility that makes the Windows taskbar translucent/transparent on Windows 10.

You can see examples of the customizations you can make in the images below:

![blur](https://i.imgur.com/r4ZJjnL.png) ![transparent](https://i.imgur.com/eLGTtwp.png) ![acrylic](https://i.imgur.com/M15IPJW.png)

## Features

- Advanced **color picker** supporting alpha and live preview to change the taskbar's color.
- **Taskbar states** (choose one - color can be customized on every state except Normal):
  - **Blur**: Will make the taskbar slightly blurred.
  - **Clear**: Transparent taskbar.
  - **Normal**: Regular Windows style. (as if TranslucentTB was not running)
  - **Opaque**: No transparency.
  - **Acrylic**: Will give the taskbar an appearance similar to Microsoft's Fluent Design guidelines.
- **Dynamic** modes (these can be used together and each of them provides a taskbar state and color you can customize):
  - **Dynamic Windows**: Will change the taskbar to a different appearance if a window is currently maximised.
  - **Dynamic Start Menu**: Will change the taskbar appearance when the start menu is opened.
  - **Dynamic Cortana**: Will change the taskbar appearance when Cortana (or the search menu if Cortana is disabled) is open.
  - **Dynamic Timeline/Task View**: Will change the taskbar apperance when the Timeline (or Task View on older builds) is open.
- Ability to **show or hide the Aero Peek** button. Can be customized **at will** or **dynamic**.

You can see it in action [here](https://gfycat.com/TidyFelineCrownofthornsstarfish) (short) and [here](https://gfycat.com/ConsciousCriminalDassie) (longer).

## Download

You can download the program freely from the [Microsoft Store](https://www.microsoft.com/store/apps/9PF4KZ2VN4W9) and take advantage of its features like background auto-updates and settings sync.

If you prefer a classical download, you can do so [via the releases tab](https://github.com/TranslucentTB/TranslucentTB/releases).

If you want to get the latest bleeding edge build, you can grab it over at the [AppVeyor page](https://ci.appveyor.com/project/sylveon/translucenttb) (`Configuration: Release` > `Artifacts` > `TranslucentTB-setup.exe`). Note that these build may not work, or include features that are partially complete. Use at your own risk.

## Configuration

Most of the program settings can be changed by right-clicking the tray icon and navigating the context menu.

Configuration files are located in `%AppData%\TranslucentTB` and consists of two files: `config.cfg` and `dynamic-ws-exclude.csv`. The main configuration file uses a simple key-value format similar to `.ini` files using semicolon-based comments. The blacklist uses a modification of the `.csv` format supporting comments using semicolons. See the files themselves for more details.

The tray icon's context menu has an advanced section that includes multiple configuration-related actions: save, reload, edit, return to defaults. Note that editing the file saves the current configuration beforehand and configuration is automatically reloaded when the file is edited on disk.

### Portable mode

Creating an empty file named `portable` (without a file extension) in the same folder than the program will make it run in portable mode. Configuration files will be located in a subfolder of the program installation folder named `config`.

### Command line arguments

TranslucentTB accepts command line arguments that change the configuration of the program. Run `TranslucentTB.exe --help` to get the list of supported arguments. They initially override any setting specified in the configuration file.

Settings changed via command-line arguments are later persisted on disk when the configuration is saved, or lost when the configuration is reloaded from disk.

### Add to Startup

To add TranslucentTB to startup, check the "Open at boot" entry in the TranslucentTB tray icon's context menu. If it is grayed out, TranslucentTB startup has been disabled from within the Task Manager, Settings app or by your organization.

## Donations

[We have a Liberapay!](https://liberapay.com/TranslucentTB/) Don't hesitate to donate if you appreciate TranslucentTB and would like to support our work.

## Security

Some antiviruses are over eager, so they might flag this program as malicious. IT IS NOT! Over 400k users have downloaded this program safely. The source is open, you can [compile it yourself](CONTRIBUTING.md#Building_from_source), and I welcome any and all security reviews.

## Thanks

TranslucentTB is a team effort! It is the result of the collective efforts of many people:

- [@ethanhs](https://github.com/ethanhs),
- [@sylveon](https://github.com/sylveon),
- [@MrAksel](https://github.com/MrAksel),
- [@denosawr](https://github.com/denosawr),
- [@PFCKrutonium](https://github.com/PFCKrutonium),
- and last but not least, all of [our contributors](https://github.com/TranslucentTB/TranslucentTB/graphs/contributors)!

Thanks to [@dAKirby309](https://github.com/dAKirby309) for making the icon! You can find more of his stuff on [his DeviantArt profile](https://dakirby309.deviantart.com/).

### Similar programs

If you are looking for something that modifies more than just the taskbar, there are several programs out there.

[Taskbar Tools](https://github.com/Elestriel/TaskbarTools) is a similar program written in C#. However, it seems to be unmaintaned.

You may have seen similar translucency abilities from programs such as StartIsBack, Start10 and the now defunct Classic Shell. All of these are great programs, but I don't need the start-replacement features, so I wrote this.
TranslucentTB also allows for more customizability over the taskbar with features such as Dynamic Windows, Dynamic Peek and Dynamic Start that these programs don't have. The storage and memory impact is also lesser.

### License

This program is free (as in speech) software under the GPLv3. Please see the [LICENSE.md](LICENSE.md) file for more information.