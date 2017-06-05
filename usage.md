## Usage
This program modifies the appearance of the windows taskbar.
You can modify its behaviour by using the following parameters when launching the program:

Option | Explanation
------------ | -------
--blur              | will make the taskbar a blurry overlay of the background (default).
--opaque            | will make the taskbar a solid color specified by the tint parameter.
--transparent       | will make the taskbar a transparent color specified by the tint parameter. The value of the alpha channel determines the opacity of the taskbar.
--tint COLOR        | specifies the color applied to the taskbar. COLOR is 32 bit number in hex format, see explanation below.
--dynamic-ws STATE  | will make the taskbar transparent when no windows are maximised in the current monitor, otherwise blurry. State can be from: (blur, opaque, tint). Blur is default.
--dynamic-start     | will make the taskbar return to it's normal state when the start menu is opened, current setting otherwise.
--exclude-file FILE | CSV-format file to specify applications to exclude from dynamic-ws (By default it will attempt to load from dynamic-ws-exclude.csv)
--save-all          | will save all of the above settings into config.cfg on program exit.
--config FILE       | will load settings from a specified configuration file. (if this parameter is ignored, it will attempt to load from config.cfg)
--help              | Displays this help message.
--startup           | Adds TranslucentTB to startup, via changing the registry.
--no-tray           | will hide the taskbar tray icon.

### Color format
The color parameter is interpreted as a three or four byte long number in hexadecimal format that 
describes the four color channels 0xAARRGGBB ([alpha,] red, green and blue). These look like this: 
0x80fe10a4 (the '0x' is optional). You often find colors in this format in the context of HTML and 
web design, and there are many online tools to convert from familiar names to this format. These 
tools might give you numbers starting with '#', in that case you can just remove the leading '#'. 
You should be able to find online tools by searching for "color to hex" or something similar. 
If the converter doesn't include alpha values (opacity), you can append them yourself at the start 
of the number. Just convert a value between 0 and 255 to its hexadecimal value before you append it

### Examples
```
# start with Windows, start transparent
TranslucentTB.exe --startup --transparent --save-all
# run dynamic windows mode, with the supplied color
TranslucentTB.exe --tint 80fe10a4 --dynamic-ws tint
# Will be normal when start is open, transparent otherwise.
TranslucentTB.exe --dynamic-start
```