#ifndef CFILE_H
#define CFILE_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

#ifdef CFILE_STATIC
#define CFILEAPI static
#else
#define CFILEAPI extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

CFILEAPI int cfile_load(CFILE* file, const char* filename);
CFILEAPI int cfile_get_int(const char* key, CFILE file);
CFILEAPI int cfile_get_string(const char* key, CFILE file, char* buffer);
CFILEAPI float cfile_get_float(const char* key, CFILE file);
CFILEAPI int cfile_free(CFILE file);

#ifdef __cplusplus
}
#endif

#ifdef CFILE_IMPLEMENTATION

/*
* Value Type
*/

typedef enum {
    TYPE_STRING,
    TYPE_FLOAT,
    TYPE_INT
} ValueType;

typedef struct {
    ValueType type;
    union {
        char* s;
        float f;
        int i;
    } data;
} Value;

/*
* Hashmap
*/

typedef struct {
    char* key;
    Value val;
    int used;
} HashEntry;

typedef struct {
    HashEntry* entries;
    int capacity;
    int count;
} HashTable;

static uint64_t fnv1a(const char* key) {
    uint64_t hash = 14695981039346656037ULL;
    while (*key) {
        hash ^= (unsigned char)(*key++);
        hash *= 1099511628211ULL;
    }
    return hash;
}

static HashTable* smap__create(int capacity) {
    HashTable* table = (HashTable*)malloc(sizeof(HashTable));
    table->capacity = capacity;
    table->count = 0;
    table->entries = (HashEntry*)calloc(capacity, sizeof(HashEntry));
    return table;
}

static void smap__set(HashTable* table, const char* key, Value val) {
    uint64_t hash = fnv1a(key);
    int idx = hash % table->capacity;
    while (table->entries[idx].used) {
        if (strcmp(table->entries[idx].key, key) == 0) {
            if (table->entries[idx].val.type == TYPE_STRING)
                free(table->entries[idx].val.data.s);
            table->entries[idx].val = val;
            return;
        }
        idx = (idx + 1) % table->capacity;
    }
    table->entries[idx].key = strdup(key);
    table->entries[idx].val = val;
    table->entries[idx].used = 1;
    table->count++;
}

static int smap__get(HashTable* table, const char* key, Value* out_val) {
    uint64_t hash = fnv1a(key);
    int idx = hash % table->capacity;
    int start = idx;
    while (table->entries[idx].used) {
        if (strcmp(table->entries[idx].key, key) == 0) {
            *out_val = table->entries[idx].val;
            return 1;
        }
        idx = (idx + 1) % table->capacity;
        if (idx == start) break;
    }
    return 0;
}

static void smap__free(HashTable* table) {
    for (int i = 0; i < table->capacity; ++i) {
        if (table->entries[i].used) {
            free(table->entries[i].key);
            if (table->entries[i].val.type == TYPE_STRING) {
                free(table->entries[i].val.data.s);
            }
        }
    }
    free(table->entries);
    free(table);
}

/*
* Cfile Parser
*/

typedef struct {
    char* filebuf;
    HashTable* table;
} CFILE;

#define IS_WHITE_SPACE(c) ((c) == ' ' || (c) == '\t')

static void trim_trailing_whitespace(char* str) {
    int len = (int)strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }
}

CFILEAPI int cfile_load(CFILE* file, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return 0;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    file->filebuf = (char*)malloc(size + 1);
    if (!file->filebuf) {
        fclose(f);
        return 0;
    }

    fread(file->filebuf, 1, size, f);
    file->filebuf[size] = '\0';
    fclose(f);

    file->table = smap__create(128);
    char* line = strtok(file->filebuf, "\n");
    while (line) {
        // Trim CR
        line[strcspn(line, "\r")] = '\0';

        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\0') {
            line = strtok(NULL, "\n");
            continue;
        }

        // Find key and value
        char* delim = line;
        while (*delim && !isspace((unsigned char)*delim)) delim++;
        if (*delim) {
            *delim = '\0';
            char* key = line;

            delim++;
            while (*delim && isspace((unsigned char)*delim)) delim++;
            char* value = delim;

            trim_trailing_whitespace(value);

            Value v;
            if (strchr(value, '.') != NULL) {
                v.type = TYPE_FLOAT;
                v.data.f = (float)atof(value);
            } else if (isdigit((unsigned char)value[0]) || value[0] == '-') {
                v.type = TYPE_INT;
                v.data.i = atoi(value);
            } else {
                v.type = TYPE_STRING;
                v.data.s = strdup(value);
            }

            smap__set(file->table, key, v);
        }

        line = strtok(NULL, "\n");
    }

    return 1;
}

CFILEAPI float cfile_get_float(const char* key, CFILE file) {
    Value val;
    if (smap__get(file.table, key, &val) && val.type == TYPE_FLOAT)
        return val.data.f;
    return 0.0f;
}

CFILEAPI int cfile_get_int(const char* key, CFILE file) {
    Value val;
    if (smap__get(file.table, key, &val) && val.type == TYPE_INT)
        return val.data.i;
    return 0;
}

CFILEAPI int cfile_get_string(const char* key, CFILE file, char* buffer) {
    Value val;
    if (smap__get(file.table, key, &val) && val.type == TYPE_STRING) {
        strcpy(buffer, val.data.s);
        return 1;
    }
    return 0;
}

CFILEAPI int cfile_free(CFILE file) {
    if (file.filebuf) free(file.filebuf);
    if (file.table) smap__free(file.table);
    return 1;
}

#endif // CFILE_IMPLEMENTATION

#endif // CFILE_H
