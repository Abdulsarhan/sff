#define SFF_IMPLEMENTATION
#include "sff.h"

/*
The basic syntax for an sff file is like so:

# comments start with a pound sign and can be placed anywhere on a line so long as they are not placed in between key value pairs.

key value


Where a key is a string that can contain letters, hyphens and underscores, and a value has to be a 32 bit int or float.

More concrete example:

# sff file for settings

screen_width 1920
screen_height 1080
FOV 91.1

*/

/* Replace this with your own path to fileformat.sff, or whichever sff file you want. */
#define FILE_PATH "C:\\Users\\john\\Desktop\\fileformat.sff"

int main() {

	SFFFILE file = load_sff_file(FILE_PATH);

	int value = sff_get_int("players_alive", &file);
	sff_free(&file);
	printf("%d\n", value);

	return 0;
}