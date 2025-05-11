#include <stdio.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __LINUX__
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#endif

#ifdef __LINUX__

int GetFileHandleLinux(const char* filePath) {
	int fd = open(filePath, O_RDONLY);
	if (fd < 0) {
		perror("ERROR: Failed to open file");
	}
	return fd; // return file descriptor (not a pointer)
}

off_t GetFileSizeLinux(int fileDescriptor) {
	struct stat st;
	if (fstat(fileDescriptor, &st) != 0) {
		perror("ERROR: Failed to get file size");
		close(fileDescriptor);
		return 0;
	}
	return st.st_size;  // `off_t` is the Linux equivalent of `LARGE_INTEGER.QuadPart`
}

int ReadEntireFileLinux(const char* filePath, char* filebuffer, int fileDescriptor, off_t fileSize) {
	if (fileDescriptor < 0) {
		fprintf(stderr, "ERROR: Failed to open file.\n");
		return -1;
	}

	if (!filebuffer) {
		fprintf(stderr, "ERROR: Memory allocation failed.\n");
		return -1;
	}

	ssize_t bytesRead = read(fileDescriptor, filebuffer, fileSize);
	if (bytesRead < 0) {
		perror("ERROR: Failed to read file");
		return -1;
	}

	if (bytesRead != fileSize) {
		fprintf(stderr, "ERROR: File size doesn't match bytes read!\n");
		return -1;
	}

	return 0;
}
#endif

#ifdef _WIN32
HANDLE* GetFileHandleWin32(const char* filePath) {
	HANDLE file = CreateFileA(
		filePath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
    if (!file) {
		fprintf(stderr, "ERROR: Failed to get win32 file handle!\n");
		return NULL;
    }

    return file;
}

LARGE_INTEGER GetFileSizeWin32(HANDLE filehandle) {

	LARGE_INTEGER fileSize = { 0 };

	if (!GetFileSizeEx(filehandle, &fileSize)) {
		fprintf(stderr, "ERROR: Failed to get file size.\n");
		CloseHandle(filehandle);
        ZeroMemory(&fileSize, sizeof(LARGE_INTEGER));
	}
    return fileSize;
}

int ReadEntireFileWin32(const char* filePath, char* filebuffer, HANDLE filehandle, LARGE_INTEGER fileSize) {


    if (filehandle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "ERROR: Failed to open file.\n");
        return -1;
    }

    if (!filebuffer) {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        return -1;
    }

    DWORD bytesRead = 0;
    BOOL success = ReadFile(filehandle, filebuffer, fileSize.QuadPart, &bytesRead, NULL);

	if (!success) {
		fprintf(stderr, "ERROR: Failed to read file!\n");
		return -1;
	}

    if (bytesRead != fileSize.QuadPart) {
        fprintf(stderr, "ERROR: File size doesn't match bytes read!\n");
        return -1;
    }



    return 0;
}
#endif

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
static uint8_t AreStringsEqual(int count, char* str1, char* str2) {
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

typedef struct {
    char* filebuf;
    HANDLE handle;
    char* parsed_buf;
    size_t parsed_len;
}SFFFILE;

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
            if (AreStringsEqual(keylen, key, fileptr)) {
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
			if (AreStringsEqual(keylen,key,fileptr)) {
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

static SFFFILE load_sff_file(const char* filepath) {

    LARGE_INTEGER filesize = {0};
    SFFFILE file = {0};
    file.parsed_len = 0;
    file.handle = GetFileHandleWin32(filepath);
    filesize = GetFileSizeWin32(file.handle);
    file.filebuf = malloc(filesize.QuadPart); // TODO: FREE THIS!!!!!!!!!!!!!!!!!!!!!
    file.parsed_buf = malloc(filesize.QuadPart); // TODO: FREE THIS!!!!!!!!!!!!!!!!!!!!!
    if (!file.filebuf) {
		fprintf(stderr, "ERROR: failed to allocate buffer for file!\n");
        exit(-1);
    }
    ReadEntireFileWin32(filepath, (char*)file.filebuf, file.handle , filesize);
	CloseHandle(file.handle);
    char* fileptr = file.filebuf; // This pointer is garbage after the loop is done. Don't use it.

    printf("INFO: Beginning First Pass!\n");
    /* FIRST PASS */
    while ((int64_t)(fileptr - file.filebuf) != filesize.QuadPart) {
        if (*fileptr == '#') {
            while (*fileptr != '\n') {
                fileptr++;
            }
            fileptr++;
        }
        else if (IS_END_OF_LINE(*fileptr)) {
            fileptr++;
        }
        else if (IS_WHITE_SPACE(*fileptr)) {
            fileptr++;

        }
        else if (IS_LETTER(*fileptr)) {
            file.parsed_buf[file.parsed_len] = *fileptr;
            file.parsed_len++;
            fileptr++;
        }
        else if (IS_UNDERSCORE(*fileptr) || IS_HYPHEN(*fileptr)) {
            file.parsed_buf[file.parsed_len] = *fileptr;
            file.parsed_len++;
            fileptr++;
        }
        else if (IS_NUMBER(*fileptr)) {
            file.parsed_buf[file.parsed_len] = *fileptr;
            file.parsed_len++;
            fileptr++;
        }
        else if (IS_DOT(*fileptr)) {
			file.parsed_buf[file.parsed_len] = *fileptr;
			file.parsed_len++;
			fileptr++;
        }
        else {

        }

    }
    fileptr = NULL;
    file.parsed_buf[file.parsed_len] = '\0';
    printf("%s\n", file.parsed_buf);
    printf("INFO: First Pass Completed!\n");

    return file;
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

/* TODO: Implement load_sff_file, sff_get_int, sff_get_float and sff_free as big functions. Any functions that can't be made into macros will be a part of the api function.*/
int main() {
    
    SFFFILE file = load_sff_file("C:\\Users\\abdul.DESKTOP-S9KEIDK\\Desktop\\fileformat.txt");

    printf("INFO: Getting value from the file!\n");
    /* TODO: Passing in an invalid key causes a crash, fix. */
    int value = sff_get_int("players_alive", &file);
    sff_free(&file);
    printf("%d\n", value);  

    return 0;
}