#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <WinDNS.h>

char* ReadEntireFileWin32(const char* filePath, char* filebuffer, QWORD* outSize) {
    HANDLE file = CreateFileA(
        filePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (file == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "ERROR: Failed to open file.\n");
        return NULL;
    }

    LARGE_INTEGER fileSize = {0};

    if (!GetFileSizeEx(file, &fileSize)) {
        fprintf(stderr, "ERROR: Failed to get file size.\n");
        CloseHandle(file);
        return NULL;
    }

    if (!filebuffer) {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        CloseHandle(file);
        return NULL;
    }
    DWORD bytesRead = 0;
    BOOL success = ReadFile(file, filebuffer, fileSize.QuadPart, &bytesRead, NULL);

    if (!success)
        fprintf(stderr, "ERROR: Failed to read file!\n");
    CloseHandle(file);

    if (outSize) 
        *outSize = fileSize.QuadPart;
    return filebuffer;
}

static uint8_t IsCapitalLetter(char ch) {
    return ((ch) >= 'A') && ((ch) <= 'Z');
}

static uint8_t IsLowerCaseLetter(char ch) {
    return ((ch) >= 'a') && ((ch) <= 'z');
}

static uint8_t IsLetter(char ch) {
    return (IsCapitalLetter(ch) || IsLowerCaseLetter(ch));
}
static uint8_t IsEndOfLine(char ch) {
    return ((ch == '\r') || (ch == '\n'));
}

static uint8_t IsWhiteSpace(char ch) {
	return ((ch == ' ') || (ch == '\t') || (ch == '\v') || (ch == '\f'));
}

static uint8_t IsNumber(char ch) {
    return ((ch >= '0') && (ch <= '9'));
}

static uint8_t IsUnderscore(char ch) {
    return (ch == '_');
}

static uint8_t IsHyphen(char ch) {
    return (ch == '-');
}

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

static uint8_t AreCharsEqual(char ch1, char ch2) {
    return (ch1 == ch2);
}

static uint8_t IsDot(char ch) {
    return(ch == '.');
}
typedef struct {
    const char* filepath;
    char buf[1024];
    int len;
}SFFFILE;

static float SffGetFloat(const char* key, SFFFILE* file) {

    char* keyptr = key;
    char* fileptr = file->buf;
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
                while (IsNumber(*endptr) || IsDot(*endptr)) {
                    endptr++;

                }
                return strtof(fileptr, endptr); // The atoi function ignores anything that's not a number. That's why it stops before a null terminator is reached.
            }
            else {
                fileptr++;
            }
            
            


        }
        if ((fileptr - file->buf) > file->len) { // Prevents fileptr from going out of bounds if the key is bad.
            fprintf(stderr, "ERROR: Invalid Key!\n");
            return 0xBADC0DE;
            break;
        }
    }

    /* UNREACHABLE !!!*/
    return 0xBADC0DE;

}

static int SffGetInt(const char* key, SFFFILE* file) {

	char* keyptr = key;
	char* fileptr = file->buf;
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

				return atoi(fileptr); // The atoi function ignores anything that's not a number. That's why it stops before a null terminator is reached.
			}
			else {
				fileptr++;
			}




		}
		if ((fileptr - file->buf) > file->len) { // Prevents fileptr from going out of bounds if the key is bad.
			fprintf(stderr, "ERROR: Invalid Key!\n");
			return 0xBADC0DE;
			break;
		}
	}

	/* UNREACHABLE !!!*/
	return 0xBADC0DE;

}

static int LoadSffFile(SFFFILE* file) {

    char filebuf[1024];
    QWORD filesize = 0;
    file->len = 0;
    ReadEntireFileWin32(file->filepath, filebuf, &filesize);
    char* fileptr = filebuf; // This pointer is garbage after the loop is done. Don't use it.

    printf("INFO: Beginning First Pass!\n");
    /* FIRST PASS */
    while ((QWORD)(fileptr - filebuf) != filesize) {
        if (*fileptr == '#') {
            while (*fileptr != '\n') {
                fileptr++;
            }
            fileptr++;
        }
        else if (IsEndOfLine(*fileptr)) {
            fileptr++;
        }
        else if (IsWhiteSpace(*fileptr)) {
            fileptr++;

        }
        else if (IsLetter(*fileptr)) {
            file->buf[file->len] = *fileptr;
            file->len++;
            fileptr++;
        }
        else if (IsUnderscore(*fileptr) || IsHyphen(*fileptr)) {
            file->buf[file->len] = *fileptr;
            file->len++;
            fileptr++;
        }
        else if (IsNumber(*fileptr)) {
            file->buf[file->len] = *fileptr;
            file->len++;
            fileptr++;
        }
        else if (IsDot(*fileptr)) {
			file->buf[file->len] = *fileptr;
			file->len++;
			fileptr++;
        }
        else {

        }

    }
    fileptr = NULL;
    file->buf[file->len] = '\0';
    printf("%s\n", file->buf);
    printf("INFO: First Pass Completed!\n");

    return 1;
}

int main() {
    
    SFFFILE file = {0};
    file.filepath = "C:\\Users\\abdul.DESKTOP-S9KEIDK\\Desktop\\fileformat.txt";
    if (!LoadSffFile(&file)) {
        fprintf(stderr, "ERROR: Failed to load Sff file.\n");
    }

    printf("INFO: Getting value from the file!\n");
    /* TODO: Passing in an invalid key causes a crash, fix. */
    float value = SffGetFloat("enemies_killed", &file);
    printf("%f\n", value);  
    return 0;
}