# TranslucentTB

[![Liberapay patrons](https://img.shields.io/liberapay/patrons/TranslucentTB.svg)](https://liberapay.com/TranslucentTB/)
[![Join on Discord](https://discordapp.com/api/guilds/304387206552879116/widget.png?style=shield)][Discord]
[![Build Status](https://dev.azure.com/sylve0n/TranslucentTB/_apis/build/status/TranslucentTB.TranslucentTB?branchName=develop)](https://dev.azure.com/sylve0n/TranslucentTB/_build/latest?definitionId=4&branchName=develop)
[![CodeFactor](https://www.codefactor.io/repository/github/translucenttb/translucenttb/badge/develop)](https://www.codefactor.io/repository/github/translucenttb/translucenttb/overview/develop)

![Microsoft Store App Awards 2022 - Community Choice Award: Open Platform (runner up)](https://user-images.githubusercontent.com/6440374/180880766-4380b2cf-4d9e-4d07-8986-a9b34cb6244a.png#gh-dark-mode-only)![Microsoft Store App Awards 2022 - Community Choice Award: Open Platform (runner up)](https://user-images.githubusercontent.com/6440374/180880839-355c472c-0b7a-4aae-88e5-0234001cb281.png#gh-light-mode-only)

A lightweight (uses a few MB of RAM and almost no CPU) utility that makes the Windows taskbar translucent/transparent on Windows 10 and Windows 11.

## Features

- Advanced **color picker** supporting alpha and live preview to change the taskbar's color.
- **Taskbar states** (choose one - color can be customized on every state except Normal):
  - **Normal**: Regular Windows style. (as if TranslucentTB was not running)
  - **Opaque**: Tinted taskbar, without transparency.
  - **Clear**: Tinted taskbar.
  - **Blur**: Will make the taskbar slightly blurred. Windows 10 and Windows 11 build 22000 only.
  - **Acrylic**: Will give the taskbar an appearance similar to Microsoft's Fluent Design guidelines.
- **Dynamic** modes (these can be used together and each of them provides a taskbar state and color you can customize):
  - **Visible window**: Will change the taskbar to a different appearance if a window is currently open on the desktop.
  - **Maximized window**: Will change the taskbar to a different appearance if a window is currently maximised.
  - **Start opened**: Will change the taskbar appearance when the start menu is opened.
  - **Search opened**: Will change the taskbar appearance when the search menu (previously Cortana) is open.
  - **Task View opened**: Will change the taskbar apperance when the Task View (previously Timeline) is open.
- On Windows 10, ability to **show or hide the Aero Peek button** depending on the currently active dynamic mode.
- On Windows 11, ability to **show or hide the taskbar line** depending on the currently active dynamic mode.
- Compatible with [RoundedTB](https://github.com/torchgm/RoundedTB)!
- Compatible with [ExplorerPatcher](https://github.com/valinet/ExplorerPatcher)!

## Screenshots

<img src="https://i.imgur.com/QbG7KQA.png" alt="windows 11 acrylic" width="243"> <img src="https://i.imgur.com/zabZ52s.png" alt="windows 11 clear" width="243">

![windows 10 acrylic](https://i.imgur.com/M15IPJW.png) ![windows 10 clear](https://i.imgur.com/eLGTtwp.png) ![windows 10 blur](https://i.imgur.com/r4ZJjnL.png)

## Download

[<img src="https://get.microsoft.com/images/en-us%20dark.svg" alt="Get it from Microsoft" height="104">](https://apps.microsoft.com/store/detail/9PF4KZ2VN4W9)

You can download the program for free from the [Microsoft Store](https://www.microsoft.com/store/apps/9PF4KZ2VN4W9) and take advantage of its features like background auto-updates.

Alternatively, you can download `TranslucentTB.appinstaller` [via the releases tab](https://github.com/TranslucentTB/TranslucentTB/releases) and open it to install the app.

A portable version of the app is also available [on GitHub releases](https://github.com/TranslucentTB/TranslucentTB/releases) as `TranslucentTB.zip`, but this version only works on Windows 11.

If you want to get the latest bleeding edge build, you can grab it over at the [Azure Pipelines page](https://dev.azure.com/sylve0n/TranslucentTB/_build?definitionId=4). Note that these builds may not work, or include features that are partially complete. Use at your own risk.

## Add to Startup

To add TranslucentTB to startup, check the "Open at boot" entry in the TranslucentTB tray icon's context menu. If it is grayed out, TranslucentTB startup has been disabled by your organization.

Portable versions can be added to startup by creating a shortcut to the executable in `%AppData%\Microsoft\Windows\Start Menu\Programs\Startup`.

## Donations and contributions

[We have a Liberapay!](https://liberapay.com/TranslucentTB/) Don't hesitate to donate if you appreciate TranslucentTB and would like to support our work.

If you want to contribute to the source code, we have [a how-to contribute guide](CONTRIBUTING.md).

## Security

Some antiviruses are over eager, so they might flag this program as malicious. IT IS NOT! Over 10M users have downloaded this program safely. The source is open, you can [compile it yourself](CONTRIBUTING.md#building-from-source), and we welcome any and all security reviews.

## Thanks

TranslucentTB is a team effort! It is the result of the collective efforts of many people:

- [@ethanhs](https://github.com/ethanhs),
- [@sylveon](https://github.com/sylveon),
- [@MrAksel](https://github.com/MrAksel),
- [@denosawr](https://github.com/denosawr),
- and last but not least, all of [our contributors](https://github.com/TranslucentTB/TranslucentTB/graphs/contributors)!

Thanks to [@dAKirby309](https://github.com/dAKirby309) for making the icon! You can find more of his stuff on [his DeviantArt profile](https://dakirby309.deviantart.com/).

### License

This program is free (as in speech) software under the GPLv3. Please see [the license file](LICENSE.md) for more.

[Discord]: https://discord.gg/TranslucentTB
