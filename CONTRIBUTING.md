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

Now that you have the source, you will need Visual Studio 2022. [You can get the free community edition here](https://visualstudio.microsoft.com/downloads/).
Install the following workloads:

- Desktop development with C++
- Universal Windows Platform development

You also need to install the following individual components:

- Windows 11 SDK (10.0.22621.0)
- If building for x64: MSVC v143 - VS 2022 C++ x64/x86 Spectre-mitigated libs (Latest)
- If building for ARM64: MSVC v143 - VS 2022 C++ ARM64 Spectre-mitigated libs (Latest)

### 3 - Install dependencies

We use vcpkg to manage our dependencies. You will need to [install it](https://vcpkg.io/en/getting-started.html).

Once you have that installed, open a terminal and execute these lines (replacing `$PATH_TO_TTB` by the location where you cloned TranslucentTB in step 1)
```sh
vcpkg install --triplet x64-windows --overlay-ports=$PATH_TO_TTB\vcpkg\ports --head gtest member-thunk rapidjson spdlog wil
vcpkg install --triplet x64-windows-static --overlay-ports=$PATH_TO_TTB\vcpkg\ports --head detours wil
vcpkg integrate install
```
`gtest` can be ommitted if you don't intend to run the unit tests. Change the triplet to `arm64-windows` and `arm64-windows-static` if building for ARM64.

### 4 - Building and running the app

Open the solution file in Visual Studio 2022. Set the AppPackage project as the startup project (right-click it in the Solution Explorer, then hit "Set as startup project").

Once this is done, you should be able to hit play, let the solution build (takes a couple minutes on a decent machine), and the app will launch.

## Translating TranslucentTB

Now that you've built the source, one of the things you can do is translating TranslucentTB in another language.

### 1 - Find your language's identifier

In order to translate TranslucentTB in your language, you will have to identify your language's identifer. It is composed of the [ISO 639-1 code](https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes) combined with a dash and the two letter identifier of your country. For example, Chinese is `zh-CN` and British English is `en-UK`. If it is correct, it should be present in the [LCID Structure] documentation.

### 2 - Create new language resources

From a file explorer:

- Duplicate the folder `Xaml\Strings\en-US` and rename it to use the language identifier found in step 1.
- Duplicate the folder `AppPackage\Strings\en-US` and rename it to use the language identifier found in step 1.
- Duplicate the file `TranslucentTB\resources\language\TranslucentTB.en-US.rc2`, replacing `en-US` by your language identifer.

Go back to Visual Studio's Solution Explorer:

- In the Xaml project, right-click on Resources > Strings and select Add > New Filter. Use the language identifer as the filter name. Then right-click this filter, select Add > Existing item, and add the `Resources.resw` file corresponding to your language.
- Select the AppPackage project, then in the top bar of the Solution Explorer click on Show All Files. Right-click on the `Resources.resw` file for your language, then select Include In Project.
- In the TranslucentTB project, right-click on Resource Files then select Add > Existing item to add the `rc2` file corresponding to your language.

### 3 - Translate

Open both `Resources.resw` files for your language by double-clicking them, and translate all the text available in the Value columns.

Open the `rc2` file for your language by right-clicking it and selecting View Code. Replace the language identifier after `LANGUAGE`, within `VarFileInfo`, and within `StringFileInfo` (only the first 4 characters of `040904b0`). You can find the macro corresponding to your language and sublanguage in the `winnt.h` header, while you can find the hexadecimal value on the [LCID Structure] documentation.

Then, translate the strings in that file as well.

### 4 - Test

Once you are done, you can launch the app and check what it looks like. If everything checks out okay, send a pull request and we'll gladly take a look at it!

[LCID Structure]: https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-lcid/63d3d639-7fd2-4afb-abbe-0d5b5551eef8
[Discord]: https://discord.gg/TranslucentTB
