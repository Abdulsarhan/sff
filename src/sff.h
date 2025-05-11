#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
	char* filebuf;
	char* parsed_buf;
	size_t parsed_len;
} SFFFILE;

static SFFFILE load_sff_file(const char* file_path);
static float sff_get_float(const char* key, SFFFILE* file);
static int sff_get_int(const char* key, SFFFILE* file);
static int sff_free(const SFFFILE* file);

#ifdef SFF_IMPLEMENTATION

#define IS_CAPITAL_LETTER(ch)  (((ch) >= 'A') && ((ch) <= 'Z'))
#define IS_LOWER_CASE_LETTER(ch)  (((ch) >= 'a') && ((ch) <= 'z'))
#define IS_LETTER(ch)  (IS_CAPITAL_LETTER(ch) || IS_LOWER_CASE_LETTER(ch))
#define IS_END_OF_LINE(ch)  (((ch) == '\r') || ((ch) == '\n'))
#define IS_WHITE_SPACE(ch)  (((ch) == ' ') || ((ch) == '\t') || ((ch) == '\v') || ((ch) == '\f'))
#define IS_NUMBER(ch)  (((ch) >= '0') && ((ch) <= '9'))
#define IS_UNDERSCORE(ch)  ((ch) == '_')
#define IS_HYPHEN(ch)  ((ch) == '-')
#define IS_DOT(ch) ((ch) == '.')
#define ARE_CHARS_EQUAL(ch1, ch2) ((ch1) == (ch2))

static SFFFILE load_sff_file(const char* file_path) {
	SFFFILE file = { 0 };

	/* OPEN FILE */
	FILE* handle = fopen(file_path, "rb");
	if (!handle) {
		fprintf(stderr, "ERROR: Failed to open file: %s\n", file_path);
		exit(EXIT_FAILURE);
	}

	/* GET FILE SIZE */
	fseek(handle, 0, SEEK_END);
	long file_size = ftell(handle);
	rewind(handle);

	if (file_size <= 0) {
		fprintf(stderr, "ERROR: Invalid file size.\n");
		fclose(handle);
		exit(EXIT_FAILURE);
	}

	/* ALLOCATE BUFFERS */
	file.filebuf = malloc(file_size);
	file.parsed_buf = malloc(file_size); // Max possible length
	if (!file.filebuf || !file.parsed_buf) {
		fprintf(stderr, "ERROR: Memory allocation failed.\n");
		fclose(handle);
		exit(EXIT_FAILURE);
	}

	/* READ FILE INTO MEMORY */
	size_t read = fread(file.filebuf, 1, file_size, handle);
	fclose(handle);
	handle = NULL;

	if (read != (size_t)file_size) {
		fprintf(stderr, "ERROR: File read incomplete (%zu of %ld bytes).\n", read, file_size);
		free(file.filebuf);
		free(file.parsed_buf);
		exit(EXIT_FAILURE);
	}

	/* FIRST PASS - PARSE FILE */
	printf("INFO: Beginning First Pass!\n");

	char* file_ptr = file.filebuf;
	char* file_end = file.filebuf + file_size;

	while (file_ptr < file_end) {
		if (*file_ptr == '#') {
			while (file_ptr < file_end && *file_ptr != '\n') file_ptr++;
			if (file_ptr < file_end) file_ptr++;
		}
		else if (IS_END_OF_LINE(*file_ptr) || IS_WHITE_SPACE(*file_ptr)) {
			file_ptr++;
		}
		else if (IS_LETTER(*file_ptr) || IS_NUMBER(*file_ptr) || IS_UNDERSCORE(*file_ptr)
			|| IS_HYPHEN(*file_ptr) || IS_DOT(*file_ptr)) {
			file.parsed_buf[file.parsed_len++] = *file_ptr++;
		}
		else {
			file_ptr++;
		}
	}

	file.parsed_buf[file.parsed_len] = '\0';
	printf("%s\n", file.parsed_buf);
	printf("INFO: First Pass Completed!\n");

	return file;
}

static float sff_get_float(const char* key, SFFFILE* file) {

    char* keyptr = key;
    char* fileptr = file->parsed_buf;
    char* endptr = NULL;
    int keylen = 0;
    while (*keyptr != '\0') {
        keylen++;
        keyptr++;
    }
    
    while (*key != '\0') { // While we haven't hit the null term for the key.
        if (*key != *fileptr) {
            fileptr++;
        }
        else { // first char of the key matches first char of the fileptr. Compare the strings to see if the whole key string matches the part of the file we want.
			int match;
			for (int i = 0; i < keylen; i++) {
				if (key == NULL || fileptr == NULL)
					match = 0;
				if (*key != *fileptr) {
					match = 0;
				}
				key++;
				fileptr++;
			}
			match = 1;
            if (match) {
                while (*fileptr == *key) { 
                    fileptr++;
                    key++;
                }
                endptr = fileptr;
                while (IS_NUMBER(*endptr) || IS_DOT(*endptr)) {
                    endptr++;

                }
                return strtof(fileptr, endptr); // The atoi function ignores anything that's not a number. That's why it stops before a null terminator is reached.
            }
            else {
                fileptr++;
            }
            
        }
        if ((fileptr - file->parsed_buf) > file->parsed_len) { // Prevents fileptr from going out of bounds if the key is bad.
            fprintf(stderr, "ERROR: Invalid Key!\n");
            return 0xBADC0DE;
            break;
        }
    }

    /* UNREACHABLE !!!*/
    return 0xBADC0DE;

}

static int sff_get_int(const char* key, SFFFILE* file) {

	char* keyptr = key;
	char* fileptr = file->parsed_buf;
	int keylen = 0;
	while (*keyptr != '\0') {
		keylen++;
		keyptr++;
	}

	while (*key != '\0') { // While we haven't hit the null term for the key.
		if (*key != *fileptr) {
			fileptr++;
		}
		else { // first char of the key matches first char of the fileptr. Compare the strings to see if the whole key string matches the part of the file we want.
			int match;
			for (int i = 0; i < keylen; i++) {
				if (key == NULL || fileptr == NULL)
					match = 0;
				if (*key != *fileptr) {
					match = 0;
				}
				key++;
				fileptr++;
			}
			match = 1;
			if (match) {
				while (*fileptr == *key) {
					fileptr++;
					key++;
				}

				return atoi(fileptr); // The atoi function ignores anything that's not a number. That's why it stops before a null terminator is reached.
			}
			else {
				fileptr++;
			}

		}
		if ((fileptr - file->parsed_buf) > file->parsed_len) { // Prevents fileptr from going out of bounds if the key is bad.
			fprintf(stderr, "ERROR: Invalid Key!\n");
			return 0xBADC0DE;
			break;
		}
	}

	/* UNREACHABLE !!!*/
	return 0xBADC0DE;

}

static int sff_free(const SFFFILE* file) {
    if (!file->filebuf || !file->parsed_buf) {
        fprintf(stderr, "ERROR: tried to free SFF file that was either not created or already freed!\n");
        exit(-1);
    }
    free(file->filebuf);
    free(file->parsed_buf);
    return 0;
}

#undef IS_CAPITAL_LETTER
#undef IS_LOWER_CASE_LETTER
#undef IS_LETTER
#undef IS_END_OF_LINE
#undef IS_WHITE_SPACE
#undef IS_NUMBER
#undef IS_UNDERSCORE
#undef IS_HYPHEN
#undef IS_DOT
#undef ARE_CHARS_EQUAL

#endif
