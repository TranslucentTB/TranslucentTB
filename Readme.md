# TranslucentTB

[![Build status](https://ci.appveyor.com/api/projects/status/9yym3vr6s5gc7vk3/branch/master?svg=true)](https://ci.appveyor.com/project/sylveon/translucenttb/branch/master)
[![Join on Discord](https://img.shields.io/discord/304387206552879116.svg)][Discord]
[![Join on Gitter](https://badges.gitter.im/TranslucentTB/Lobby.svg)][Gitter]
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
  - **Fluent**: Windows 10 April 2018 update and up only. Will give the taskbar an appearance similar to Microsoft's Fluent Design guidelines.
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

## Add to Startup

To add TranslucentTB to startup, check the "Open at boot" entry in the TranslucentTB tray icon's context menu. If it is grayed out, TranslucentTB startup has been disabled from within the Task Manager or by your organization.

## Donations

[We have a Liberapay!](https://liberapay.com/TranslucentTB/) Don't hesitate to donate if you appreciate TranslucentTB and would like to support our work.

## Security

Some antiviruses are over eager, so they might flag this program as malicious. IT IS NOT! Over 200k users have downloaded this program safely. The source is open, you can compile it yourself, and I welcome any and all security reviews.

Speaking of compiling...

## Building from source

You can checkout one of the available branches. However, it is recommended to use `master`, as the code here is stable and has been passed through peer review.

Via [git](https://git-scm.com):

```sh
$ git clone -b [branch-you-want] https://github.com/TranslucentTB/TranslucentTB
Cloning into 'TranslucentTB'...
remote: Counting objects: 909, done.
remote: Compressing objects: 100% (40/40), done.
remote: Total 909 (delta 44), reused 61 (delta 35), pack-reused 834
Receiving objects: 100% (909/909), 383.94 KiB | 2.78 MiB/s, done.
Resolving deltas: 100% (624/624), done.
```

You can also download a zip archive of each branch by clicking on the `Clone or download` button while browsing the branch's files.

Now that you have the source, you will need Visual Studio 2017. [You can get the free community edition here](https://www.visualstudio.com/vs/community/).
Check the following workloads:

- Desktop development with C++
- .NET desktop development

You also need to install the following individual components:

- Any of the VC++ 2017 toolsets (latest prefered)
- Windows 10 SDK (10.0.17134.0)
- .NET Framework 4.6.2 SDK
- .NET Framework 4.6.2 targeting pack

You also need the [Clang compiler for Windows](http://releases.llvm.org/download.html) and [Inno Setup](http://jrsoftware.org/isdl.php).

<!-- markdownlint-disable MD033 -->
Once you have that installed, open `TranslucentTB.sln`, and press <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>B</kbd> to build the solution.
<!-- markdownlint-enable MD033 -->

The output will be in either the Debug or Release folder (depending on which solution configuration is currently active).

To build the desktop installer, run the DesktopInstallerBuilder project.

To build the Microsoft Store app package, build the solution with the Store configuration.

## Contributing

If you would like to contribute, everyone is welcome to! If you are considering a major feature, need guidance, or want to talk an idea out, don't hesitate to jump on [Discord], [Gitter], or file an issue here. The main contributors are often on [Discord], [Gitter] and GitHub, so we should reply fairly quickly.
At this time we have no plans of expanding this beyond the taskbar.

When contributing, please respect the style used by the codebase. Quick rundown:

- Allman braces everywhere, even on one line blocks:

  ```cpp
  // Bad!
  if (condition) {
      statement;
  }
  
  // Bad!
  if (condition) statement;
  
  // Bad!
  if (condition)
      statement;
  
  // Good!
  if (condition)
  {
      statement;
  }
  ```

- The only exception to this rule is the opening brace of a class, enumeration, namespace or structure, in which K&R braces apply:

  ```cpp
  class Foo {
      // content
  };
  
  struct Bar {
      // content
  };
  
  namespace Baz {
      // content
  }

  enum Foobar {
      // content
  };
  ```

- lvalue, rvalue and pointer qualifiers are next to the variable name:

  ```cpp
  std::wstring &foo;
  std::wstring &&bar;
  std::wstring *baz;
  ```

- Indentation style is 4 spaces large tabs, and your editor should enforce it with this repo's `.editorconfig` automatically.

When trying to debug the main program, it might seem confusing at first because the two projects listed for launch in the header are StorePackage and DesktopInstallerBuilder. Just right-click the TranslucentTB project and select "Set as startup project".

## Thanks

TranslucentTB is a team effort! It is the result of the collective efforts of many people:

- [@ethanhs](https://github.com/ethanhs),
- [@sylveon](https://github.com/sylveon),
- [@MrAksel](https://github.com/MrAksel),
- [@denosawr](https://github.com/denosawr),
- [@PFCKrutonium](https://github.com/PFCKrutonium),
- and last but not least, all of [our contributors](https://github.com/TranslucentTB/TranslucentTB/graphs/contributors)!

Thanks to [@dAKirby309](https://github.com/dAKirby309) for making the icon! You can find more of his stuff on [his DeviantArt profile](https://dakirby309.deviantart.com/).

The color picker used comes from [this great CodeProject article](https://www.codeproject.com/Articles/9207/An-HSV-RGBA-colour-picker).
We've modernized it a bit, with per-monitor high DPI awareness, faster (and hardware-accelerated) drawing as well as allowing to input any valid HTML color code or [name](https://www.w3schools.com/colors/colors_names.asp).

The picture we used for the installer screenshot is by [Michael D Beckwith](https://unsplash.com/photos/M-nHIqkO4-o) from [Unsplash](https://unsplash.com/).

We use [Inno Setup Dependency Installer](https://github.com/stfx/innodependencyinstaller) to install the Visual C++ redistribuable.

### Similar programs

If you are looking for something that modifies more than just the taskbar, there are several programs out there.

[Taskbar Tools](https://github.com/Elestriel/TaskbarTools) is a similar program written in C#. However, it seems to be unmaintaned.

You may have seen similar translucency abilities from programs such as StartIsBack, Start10 and the now defunct Classic Shell. All of these are great programs, but I don't need the start-replacement features, so I wrote this.
TranslucentTB also allows for more customizability over the taskbar with features such as Dynamic Windows, Dynamic Peek and Dynamic Start that these programs don't have. The storage and memory impact is also lesser.

### License

This program is free (as in speech) software under the GPLv3. Please see the [LICENSE.md](LICENSE.md) file for more.

[Discord]: https://discord.gg/w95DGTK
[Gitter]: https://gitter.im/TranslucentTB/Lobby
