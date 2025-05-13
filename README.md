# SET File Format.

Just a simple key-value pair file format that also supports comments.
SET stands for settings. I made this format so that you can easily save and load settings from a file.

## File Syntax

```
# A valid key can contain: letters, hyphens and underscores. Other kinds of characters are not permitted to be used in keys.
# And a value can be either a 32 bit signed int or a 32 bit float, which is big enough for most use cases.
# Ints can easily convert to bools, so you can convert the int to a bool if you want a bool.
# I might add strings in the future, but that's about it.
# The key-value pairs can have as few or as many spaces as you want. You can forgo spaces alltogether.

Basic Syntax:

# Settings

is_fullscreen 1

screenWidth 1920
screenHeight 1080

# Horizontal Mouse Sens.
mouse-sensitivity-x 1.5

# Vertical Mouse Sens.
mouse-sensitivity-y 1.5

invalid syntax:

is.fullscreen # Can't have comments in between key-value pairs. yes_is_fullscreen
```

## Basic Usage
```C
#include <stdio.h>
#include <stdbool.h>
#define SET_IMPLEMENTATION
#include "set.h"

int main() {
    SETFILE file = set_load_file("C:\\Users\\JohnDoe\\Desktop\\settings.set");

    int screen_width = set_get_int("screen_width", file);
	int screen_height = set_get_int("screen_height", file);
    bool fullscreen = set_get_int("is_fullscreen", file);
	float mousesens = set_get_float("mouse_sens", file);
	
    printf("screen_width: %d, screen_height: %d , %d", "%f", screen_width, screen_height, fullscreen, mousesens);
    set_free(file);
	return 0;
}
```
