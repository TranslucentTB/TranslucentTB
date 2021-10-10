# Language Translator's Guide

This guide is for anyone who interested to help us translate TranslucentTB. If you stuck, ask us via Discord.

## 1 - Fork the repo

On this GitHub repo, click **fork** button at top-right of the page. Then run this command:

```sh
git clone -b develop https://github.com/<your_github_username>/TranslucentTB
```

> **ATTENTION:** Don't forget to change `<your_github_username>` to your GitHub username.

## 2 - Prepare required stuffs

In order to translate the language, you have to get Visual Studio 2022 installed. Follow [Building from source](https://github.com/TranslucentTB/TranslucentTB/blob/release/CONTRIBUTING.md#building-from-source) instruction but skip the first step. Don't worry if you failed to compile the source, unless you really want to try your changes.

## 3 - Find your language BCP 47 code

To identify your language, you have to remember your language's BCP 47 code. Find it on [locale table](https://docs.microsoft.com/en-us/openspecs/office_standards/ms-oe376/6c085406-a698-4e12-9d4d-c3b0ee3dbc4a).

## 4 - Set up the manifest file

Make sure you are on Visual Studio 2022 with TranslucentTB solution loaded. At the left side, you have **Solution Explorer** (assuming you didn't change the layout). Find this item:

```
Solution 'TranslucentTB' > AppPackage > Package.appxmanifest
```

Right click on it, choose "View Code". Find something like:

```xml
  ...
  <Resources>
    <Resource Language="en-us" />
    ...
  </Resources>
  ...
```

Then add your language

```xml
  ...
  <Resources>
    <Resource Language="en-us" />
    ...
    <Resource Language="your-BCP-47-here" />
  </Resources>
  ...
```

Done! Now, save the file (CTRL+S).

## 5 - Copy & paste existing language

Use windows file explorer to navigate the repo:

```
(project folder path)\Xaml\Resources\String
```

Duplicate `en-US` folder then rename to your language's BCP 47 code.

## 6 - Register the language to the project

Now go back to Visual Studio, at the Solution Explorer, navigate to:

```
Xaml (Universal Windows) > Resources > Strings
```

Right click on "Strings" folder then choose `Add > New Filter`, name it to your language's BCP 47 code. Right click on that created folder, choose `Add > Existing Item` and select:

```
(project folder path)\Xaml\Resources\String\your-BCP-47\Resources.resw
```

Then your translation file on Solution Explorer available at:

```
Xaml (Universal Windows) > Resources > Strings > your-BCP-47 > Resources.resw
```

## 7 - It's translate time!

Simply double click this on Solution Explorer

```
Xaml (Universal Windows) > Resources > Strings > your-BCP-47 > Resources.resw
```

then translate everything at **Value** column to your language.

## 8 - Finally, deploy the changes

At Visual Studio, hit **CTRL + ~** on your keyboard. Then enter this commands:

```
git add --all
git commit -a -m "feat: Add language your_language"
git push
```

> Replace **your_language** to your language name

After that, let's pull request. There's a [nice guide](https://youtu.be/a_FLqX3vGR4?t=515) that you can follow.
