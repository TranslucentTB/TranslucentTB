# Contributing

Thank you for wanting to contribute to TranslucentTB :)

If you are considering a major feature, need guidance, or want to talk an idea out, don't hesitate to jump on [Discord], [Gitter], or file an issue on the GitHub issue tracker. The main contributors are often on [Discord], [Gitter] and GitHub, so we should reply fairly quickly.

At this time we have no plans of expanding this beyond the taskbar.

## Building from source
<!-- TODO: needs update -->

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

You can also download a zip archive of each branch by clicking `Clone or download` > `Download ZIP` while browsing the branch's files.

Now that you have the source, you will need Visual Studio 2017. [You can get the free community edition here](https://www.visualstudio.com/vs/community/).
Check the following workloads:

- Desktop development with C++
- .NET desktop development

You also need to install the following individual components:

- Any of the VC++ 2017 toolsets (latest prefered)
- Windows 10 SDK (10.0.17134.0)
- .NET Framework 4.6.2 SDK
- .NET Framework 4.6.2 targeting pack

You also need the [Clang compiler for Windows version 7 or above](http://releases.llvm.org/download.html), the [LLVM Compiler Toolchain Visual Studio addon](https://marketplace.visualstudio.com/items?itemName=LLVMExtensions.llvm-toolchain), [Inno Setup](http://jrsoftware.org/isdl.php) and [vcpkg](https://github.com/microsoft/vcpkg).

Once you have that installed, Open a terminal and execute these lines
```sh
cd path\to\TranslucentTB
vcpkg install --triplet x64-windows fmt spdlog discord-game-sdk
vcpkg install --triplet x64-windows --overlay-ports=ports --head detours gtest member-thunk rapidjson wil
vcpkg integrate install
```
`gtest` can be skipped if you don't intend to run the unit tests.

<!-- markdownlint-disable MD033 -->
Once you have that installed, open `TranslucentTB.sln`, and press <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>B</kbd> to build the solution.
<!-- markdownlint-enable MD033 -->

The output will be in either the Debug or Release folder (depending on which solution configuration is currently active).

To build the desktop installer, run the DesktopInstallerBuilder project (you need to have built the solution in both x86 and x64 with the Release configuration before).

To build the Microsoft Store app package, build the solution with the Store configuration.

## Code Style

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

## Notes

When trying to debug the main program, it might seem confusing at first because the only two projects listed for launch in the header are StorePackage and DesktopInstallerBuilder. Just right-click the TranslucentTB project and select "Set as startup project".

[Discord]: https://discord.gg/TranslucentTB
[Gitter]: https://gitter.im/TranslucentTB/Lobby
