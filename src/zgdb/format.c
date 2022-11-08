#define ZGDB_FILETYPE 0x4244475A
#define ZGDB_DEFAULT_INDEX_CAPACITY 10

#include <malloc.h>
#include "format.h"

zgdbHeader* readHeader(FILE* f) {
    zgdbHeader* header = malloc(sizeof(zgdbHeader));
    if (header) {
        fread(header, sizeof(zgdbHeader), 1, f);
    }
    return header;
}

zgdbHeader* createHeader() {
    zgdbHeader* header = malloc(sizeof(zgdbHeader));
    if (header) {
        header->fileType = ZGDB_FILETYPE;
        header->freeListOffset = 0;
        header->indexNumber = 0;
    }
    return header;
}

bool writeHeader(zgdbFile* file) {
    rewind(file->f);
    return fwrite(file->header, sizeof(zgdbHeader), 1, file->f);
}

size_t writeIndexes(zgdbFile* file, size_t count) {
    zgdbIndex index = { INDEX_NEW, 0 };
    //zgdbIndex index = { INDEX_ALIVE, 0 };
    fseek(file->f, sizeof(zgdbHeader), SEEK_SET);
    for (int i = 0; i < file->header->indexNumber; i++) {
        fseek(file->f, sizeof(zgdbIndex), SEEK_CUR);
    }
    size_t written = 0;
    for (int i = 0; i < count; i++) {
        written += fwrite(&index, sizeof(zgdbIndex), 1, file->f);
    }
    return written;
}

zgdbFile* loadFile(const char* filename) {
    zgdbFile* file = malloc(sizeof(zgdbFile));
    if (file) {
        file->f = fopen(filename, "r+b");
        if (file->f) {
            if (!(file->header = readHeader(file->f))) {
                fclose(file->f);
                free(file);
                return NULL;
            }
        } else {
            free(file);
            return NULL;
        }
    }
    return file;
}

zgdbFile* createFile(const char* filename) {
    zgdbFile* file = malloc(sizeof(zgdbFile));
    if (file) {
        file->f = fopen(filename, "w+b");
        if (file->f) {
            if ((file->header = createHeader())) {
                file->header->indexNumber += writeIndexes(file, ZGDB_DEFAULT_INDEX_CAPACITY);
                if (file->header->indexNumber != ZGDB_DEFAULT_INDEX_CAPACITY || !writeHeader(file)) {
                    fclose(file->f);
                    free(file->header);
                    free(file);
                    return NULL;
                }
            } else {
                fclose(file->f);
                free(file);
                return NULL;
            }
        } else {
            free(file);
            return NULL;
        }
    }
    return file;
}

void closeFile(zgdbFile* file) {
    if (file) {
        if (file->header) {
            free(file->header);
        }
        if (file->f) {
            fclose(file->f);
        }
        free(file);
    }
}