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

int main() {

	SFFFILE file = load_sff_file("C:\\Users\\JohnSmith\\Desktop\\fileformat.txt");

	printf("INFO: Getting value from the file!\n");
	/* TODO: Passing in an invalid key causes a crash, fix. */
	int value = sff_get_int("players_alive", &file);
	sff_free(&file);
	printf("%d\n", value);

	return 0;
}