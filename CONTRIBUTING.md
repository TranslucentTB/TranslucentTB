# Contributing

Thank you for wanting to contribute to TranslucentTB :)

If you are considering a major feature, need guidance, or want to talk an idea out, don't hesitate to jump on [Discord] or file an issue on the GitHub issue tracker. The main contributors are often on [Discord] and GitHub, so we should reply fairly quickly.

At this time we have no plans of expanding this beyond the taskbar.

## Building from source

### 1 - Clone the repo

You can checkout one of the available branches. For development, one should use `develop`. If you want to simply build it, use `release`.

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

### 2 - Install the build tools

Now that you have the source, you will need Visual Studio 2022. [You can get the free community edition here](https://visualstudio.microsoft.com/vs/preview/).
Check the following workloads:

- Desktop development with C++
- Universal Windows Platform development

You also need to install the "MSVC v142 - VS 2019 C++ x64/x86 build tools (v14.29-16.11)" component (under the Individual Components section).

We currently use the Windows 11 SDK, as it has several important bug fixes. You can get it from the [Microsoft Insider Preview SDK download page](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK).

### 3 - Install dependencies

We use vcpkg to manage our dependencies. You will need to [install it](https://vcpkg.io/en/getting-started.html).

Once you have that installed, open a terminal and execute these lines (replacing `$PATH_TO_TTB` by the location where you cloned TranslucentTB in step 1)
```sh
vcpkg install --overlay-triplets=$PATH_TO_TTB\vcpkg\triplets --triplet x64-windows-ttb fmt spdlog
vcpkg install --overlay-triplets=$PATH_TO_TTB\vcpkg\triplets --triplet x64-windows-ttb --overlay-ports=$PATH_TO_TTB\vcpkg\ports --head detours gtest member-thunk rapidjson wil
vcpkg integrate install
```
`gtest` can be ommitted if you don't intend to run the unit tests.

> **Note!**  
> If you get a message saying "Applying patch failed" when installing `detours`, you will have to check out a [vcpkg PR that has yet to be merged](https://github.com/microsoft/vcpkg/pull/19657). This message will be removed once the PR is merged. To check out the PR, run these in the folder you cloned vcpkg in:
> ```sh
> git remote add strega-nil https://github.com/strega-nil/vcpkg.git
> git fetch strega-nil minor-fixes
> git switch --track strega-nil/minor-fixes
> ```

### 4 - Building and running the app

Open the solution file in Visual Studio 2022. Set the AppPackage project as the startup project (right-click in the Solution Explorer, then hit "Set as startup project").

By default, Visual Studio attempts to build ARM64. You will want to change the solution platform to x64.

Once this is done, you should be able to hit play, let the solution build (takes a couple minute on a decent machine), and the app will launch.

> **Note!**  
> If you get an error dialog saying "The code execution cannot proceed because fmtd.dll was not found.", you will have to go and copy `<vcpkg>\installed\x64-windows-ttb\debug\bin\fmtd.dll` to `<TranslucentTB>\AppPackage\bin\x64\Debug\AppX`. [This is a bug in vcpkg](https://github.com/microsoft/vcpkg/issues/16184). Once the file is copied, you can relaunch the app from Visual Studio.

[Discord]: https://discord.gg/TranslucentTB
[Gitter]: https://gitter.im/TranslucentTB/Lobby

## Help us translate the language

We created a [guide to translate language](https://github.com/TranslucentTB/TranslucentTB/blob/release/TRANSLATOR_GUIDE.md) properly.
