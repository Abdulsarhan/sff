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
CFILEAPI int cfile_get_string(char* key, CFILE file, char* buffer);
CFILEAPI float cfile_get_float(char* key, CFILE file);
CFILEAPI int cfile_get_int(char* key, CFILE file);
CFILEAPI int cfile_free(CFILE file);

#ifdef __cplusplus
}
#endif

#ifdef CFILE_IMPLEMENTATION

#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define IS_CAPITAL_LETTER(ch)  (((ch) >= 'A') && ((ch) <= 'Z'))
#define IS_LOWER_CASE_LETTER(ch)  (((ch) >= 'a') && ((ch) <= 'z'))
#define IS_LETTER(ch)  (IS_CAPITAL_LETTER(ch) || IS_LOWER_CASE_LETTER(ch))
#define IS_END_OF_LINE(ch)  (((ch) == '\r') || ((ch) == '\n'))
#define IS_WHITE_SPACE(ch)  (((ch) == ' ') || ((ch) == '\t') || ((ch) == '\v') || ((ch) == '\f'))
#define IS_NUMBER(ch)  (((ch) >= '0') && ((ch) <= '9'))
#define IS_UNDERSCORE(ch)  ((ch) == '_')
#define IS_HYPHEN(ch)  ((ch) == '-')
#define IS_DOT(ch) ((ch) == '.')
#define IS_DOUBLEQUOTES(ch) ((ch) == '"')
#define ARE_CHARS_EQUAL(ch1, ch2) ((ch1) == (ch2))

static uint32_t cfile__are_strings_equal(size_t count, char* str1, char* str2) {

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

static char* escape_string(const char* input) {
    size_t len = strlen(input);
    size_t extra = 0;

    for (size_t i = 0; i < len; ++i) {
        if (input[i] == '"' || input[i] == '\\') {
            extra++;
        }
    }

    char* escaped = malloc(len + extra + 1);
    if (!escaped) return NULL;

    char* out = escaped;
    for (size_t i = 0; i < len; ++i) {
        if (input[i] == '"' || input[i] == '\\') {
            *out++ = '\\';
        }
        *out++ = input[i];
    }
    *out = '\0';
    return escaped;
}

static int compare_lines(const void* a, const void* b) {
    const char* line_a = *(const char**)a;
    const char* line_b = *(const char**)b;
    return strcmp(line_a, line_b);
}

CFILEAPI int cfile_save(const char* key, const char* value, const char* file_path) {
    if (!key || !value || !file_path) {
        fprintf(stderr, "ERROR: cfile_save: Invalid arguments.\n");
        return -1;
    }

    FILE* in = fopen(file_path, "r");
    if (!in && errno != ENOENT) {
        fprintf(stderr, "ERROR: cfile_save: Could not open file for reading: %s\n", file_path);
        return -1;
    }

    char** lines = NULL;
    size_t line_count = 0;
    size_t key_len = strlen(key);
    int key_found = 0;

    if (in) {
        char line[1024];
        while (fgets(line, sizeof(line), in)) {
            line[strcspn(line, "\r\n")] = 0;

            // Extract current key from line
            char* delim = strchr(line, ' ');
            if (delim && (size_t)(delim - line) == key_len && strncmp(line, key, key_len) == 0) {
                key_found = 1;

                char new_line[2048];
                int is_number = 1;
                for (const char* p = value; *p; ++p) {
                    if (!IS_NUMBER(*p) && !IS_DOT(*p) && *p != '-') {
                        is_number = 0;
                        break;
                    }
                }

                if (is_number) {
                    snprintf(new_line, sizeof(new_line), "%s %s", key, value);
                } else {
                    char* escaped = escape_string(value);
                    if (!escaped) {
                        fclose(in);
                        return -1;
                    }
                    snprintf(new_line, sizeof(new_line), "%s \"%s\"", key, escaped);
                    free(escaped);
                }

                lines = realloc(lines, sizeof(char*) * (line_count + 1));
                lines[line_count++] = strdup(new_line);
            } else {
                lines = realloc(lines, sizeof(char*) * (line_count + 1));
                lines[line_count++] = strdup(line);
            }
        }
        fclose(in);
    }

    if (!key_found) {
        char new_line[2048];

        int is_number = 1;
        for (const char* p = value; *p; ++p) {
            if (!IS_NUMBER(*p) && !IS_DOT(*p) && *p != '-') {
                is_number = 0;
                break;
            }
        }

        if (is_number) {
            snprintf(new_line, sizeof(new_line), "%s %s", key, value);
        } else {
            char* escaped = escape_string(value);
            if (!escaped) return -1;
            snprintf(new_line, sizeof(new_line), "%s \"%s\"", key, escaped);
            free(escaped);
        }

        lines = realloc(lines, sizeof(char*) * (line_count + 1));
        lines[line_count++] = strdup(new_line);
    }

    // Sort lines alphabetically by key
    qsort(lines, line_count, sizeof(char*), compare_lines);

    FILE* out = fopen(file_path, "w");
    if (!out) {
        fprintf(stderr, "ERROR: cfile_save: Could not open file for writing: %s\n", file_path);
        for (size_t i = 0; i < line_count; i++) free(lines[i]);
        free(lines);
        return -1;
    }

    for (size_t i = 0; i < line_count; i++) {
        fprintf(out, "%s\n", lines[i]);
        free(lines[i]);
    }

    free(lines);
    fclose(out);
    return 0;
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

    /* FIRST PASS */
    char* file_ptr = file.filebuf;
    char* file_end = file.filebuf + file_size;

    while (file_ptr < file_end) {
        // Skip leading whitespace and blank lines
        while (file_ptr < file_end && (IS_WHITE_SPACE(*file_ptr) || IS_END_OF_LINE(*file_ptr))) {
            file_ptr++;
        }

        // Parse each line
        while (file_ptr < file_end && !IS_END_OF_LINE(*file_ptr)) {
            if (*file_ptr == '#') {
                // Inline comment: skip to end of line
                while (file_ptr < file_end && !IS_END_OF_LINE(*file_ptr)) {
                    file_ptr++;
                }
                break; // done with this line
            }

            if (IS_LETTER(*file_ptr) || IS_NUMBER(*file_ptr) || IS_UNDERSCORE(*file_ptr)
                || IS_HYPHEN(*file_ptr) || IS_DOT(*file_ptr) || IS_DOUBLEQUOTES(*file_ptr)) {
                file.parsed_buf[file.parsed_len++] = *file_ptr;
            }

            file_ptr++;
        }

        // Skip trailing newline chars
        while (file_ptr < file_end && IS_END_OF_LINE(*file_ptr)) {
            file_ptr++;
        }
    }

    file.parsed_buf[file.parsed_len] = '\0';
    return file;
}

CFILEAPI int cfile_get_string(char* key, CFILE file, char* buffer) {

    size_t keylen = strlen(key);
    char* fileptr = file.parsed_buf;
    char* endptr = NULL;
    char* string = NULL;

    for (int i = 0; i < file.parsed_len; i++) {

        if (*key != *fileptr) {
            fileptr++;
        }
        if (cfile__are_strings_equal(keylen, key, fileptr)) {
            fileptr += keylen;
            endptr = fileptr;
            while (IS_NUMBER(*endptr) || IS_LETTER(*endptr) || IS_DOUBLEQUOTES(*endptr)) {
                endptr++;
            }
            fileptr++;
            endptr--;
            string = fileptr;
            *endptr = '\0';
            strcpy(buffer, string);
            return 0;
        }
        else {
            fileptr++;
        }

    }
    fprintf(stderr, "ERROR: cfile_get_string: Invalid key!\n");
    return 0xBADC0DE;

}

CFILEAPI float cfile_get_float(char* key, CFILE file) {

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
    fprintf(stderr, "ERROR: cfile_get_float: Invalid key!\n");
    return 0xBADC0DE;

}

CFILEAPI int32_t cfile_get_int(char* key, CFILE file) {

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
    fprintf(stderr, "ERROR: cfile_get_int: Invalid key!\n");
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
