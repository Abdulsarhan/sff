#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
	char* filebuf;
	char* parsed_buf;
	size_t parsed_len;
} CFILE;

#ifdef CFILE_STATIC
#define CFILEAPI static
#else
#define CFILEAPI extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

CFILEAPI CFILE cfile_load(const char* file_path);
CFILEAPI float cfile_get_float(const char* key, CFILE file);
CFILEAPI int cfile_get_int(const char* key, CFILE file);
CFILEAPI int cfile_free(CFILE file);

#ifdef __cplusplus
}
#endif

#ifdef CFILE_IMPLEMENTATION

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

uint32_t cfile__are_strings_equal(size_t count, char* str1, char* str2) {

	for (int i = 0; i < count; i++) {
		if (str1 == NULL || str2 == NULL)
			return 0;
		if (*str1 != *str2) {
			return 0;
		}
		str1++;
		str2++;
	}
	return 1;
}

CFILEAPI CFILE cfile_load(const char* file_path) {
	CFILE file = { 0 };

	/* OPEN FILE */
	FILE* handle = fopen(file_path, "rb");
	if (!handle) {
		fprintf(stderr, "ERROR: set_load_file: Failed to open file: %s\n", file_path);
		exit(EXIT_FAILURE);
	}

	/* GET FILE SIZE */
	fseek(handle, 0, SEEK_END);
	long file_size = ftell(handle);
	rewind(handle);

	if (file_size <= 0) {
		fprintf(stderr, "ERROR: set_load_file: Invalid file size.\n");
		fclose(handle);
		exit(EXIT_FAILURE);
	}

	/* ALLOCATE BUFFERS */
	file.filebuf = malloc(file_size);
	file.parsed_buf = malloc(file_size);
	if (!file.filebuf || !file.parsed_buf) {
		fprintf(stderr, "ERROR: set_load_file: Memory allocation failed.\n");
		fclose(handle);
		exit(EXIT_FAILURE);
	}

	/* READ FILE INTO MEMORY */
	size_t read = fread(file.filebuf, 1, file_size, handle);
	fclose(handle);
	handle = NULL;

	if (read != (size_t)file_size) {
		fprintf(stderr, "ERROR: set_load_file: File read incomplete (%zu of %ld bytes).\n", read, file_size);
		free(file.filebuf);
		free(file.parsed_buf);
		exit(EXIT_FAILURE);
	}

	/* FIRST PASS - PARSE FILE */
	//printf("INFO: Beginning First Pass!\n");

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
	// file.parsed_buf contains the file, but without any new lines or white spaces.
	file.parsed_buf[file.parsed_len] = '\0';
	//printf("%s\n", file.parsed_buf);
	//printf("INFO: First Pass Completed!\n");

	return file;
}

CFILEAPI float cfile_get_float(const char* key, CFILE file) {

	size_t keylen = strlen(key);
	char* fileptr = file.parsed_buf;
	char* endptr = NULL;

	for (int i = 0; i < file.parsed_len; i++) {

		if (*key != *fileptr) {
			fileptr++;
		}
		if (cfile__are_strings_equal(keylen, key, fileptr)) {
			fileptr += keylen;
			endptr = fileptr;
			while (IS_NUMBER(*endptr) || IS_DOT(*endptr)) {
				endptr++;
			}
			return strtof(fileptr, &endptr);

		}
		else {
			fileptr++;
		}

	}
	fprintf(stderr, "ERROR: set_get_float: Invalid key!\n");
	return 0xBADC0DE;

}

CFILEAPI int32_t cfile_get_int(const char* key, CFILE file) {

	size_t keylen = strlen(key);
	char* fileptr = file.parsed_buf;

	for (int i = 0; i < file.parsed_len; i++) {

		if (*key != *fileptr) {
			fileptr++;
		}
		if (cfile__are_strings_equal(keylen, key, fileptr)) {
			fileptr += keylen;
			return atoi(fileptr);
		}
		else {
			fileptr++;
		}

	}
	fprintf(stderr, "ERROR: set_get_int: Invalid key!\n");
	return 0xBADC0DE;

}

CFILEAPI int32_t cfile_free(CFILE file) {
    if (!file.filebuf || !file.parsed_buf) {
        fprintf(stderr, "ERROR: tried to free SFF file that was either not created or already freed!\n");
        exit(-1);
    }
    free(file.filebuf);
    free(file.parsed_buf);
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
