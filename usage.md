## Usage
This program modifies the apperance of the windows taskbar.
You can modify its behaviour by using the following parameters when launching the program:

Option | Explanation
------------ | -------
--blur        | will make the taskbar a blurry overlay of the background (default). 
--opaque      | will make the taskbar a solid color specified by the tint parameter. 
--transparent | will make the taskbar a transparent color specified by the tint parameter.  The value of the alpha channel determines the opacity of the taskbar. 
--tint COLOR  | specifies the color applied to the taskbar. COLOR is 32 bit number in hex format, see explanation below. This will not affect the blur mode. If COLOR is zero in combination with --transparent the taskbar becomes opaque and uses the selected system color scheme. 
--help        | Displays this help message.

### Color format
The color parameter is interpreted as a three or four byte long number in hexadecimal format that 
describes the four color channels ([alpha,] red, green and blue). These look like this: 
0x80fe10a4 (the '0x' is optional). You often find colors in this format in the context of HTML and 
web design, and there are many online tools to convert from familiar names to this format. These 
tools might give you numbers starting with '#', in that case you can just remove the leading '#'. 
You should be able to find online tools by searching for "color to hex" or something similar. 
If the converter doesn't include alpha values (opacity), you can append them yourself at the start 
of the number. Just convert a value between 0 and 255 to its hexadecimal value before you append it
