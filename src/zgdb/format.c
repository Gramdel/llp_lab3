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

zgdbHeader* initHeader() {
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

zgdbIndex* getIndex(zgdbFile* file, uint64_t i) {
    zgdbIndex* index = malloc(sizeof(zgdbIndex));
    if (index) {
        fseek(file->f, sizeof(zgdbHeader), SEEK_SET);
        for (int j = 0; j < i; j++) {
            fseek(file->f, sizeof(zgdbIndex), SEEK_CUR);
        }
        fread(index, sizeof(zgdbIndex), 1, file->f);
    }
    return index;
}

bool updateIndex(zgdbFile* file, uint64_t i, uint8_t* flag, uint64_t* offset) {
    fseek(file->f, sizeof(zgdbHeader), SEEK_SET);
    for (int j = 0; j < i; j++) {
        fseek(file->f, sizeof(zgdbIndex), SEEK_CUR);
    }
    size_t written = 0;
    if (flag) {
        written += fwrite(flag, sizeof(uint8_t), 1, file->f);
    } else {
        fseek(file->f, sizeof(uint8_t), SEEK_CUR);
    }
    if (offset) {
        written += fwrite(offset, sizeof(uint64_t), 1, file->f);
    }
    return ((flag == NULL) == (offset == NULL)) ? (written == 2) : (written ==
                                                                    1); // XOR, оба NULL - проверка на 2, один NULL - на 1
}

zgdbFile* loadFile(const char* filename) {
    zgdbFile* file = malloc(sizeof(zgdbFile));
    if (file) {
        file->f = fopen(filename, "r+b");
        if (file->f) {
            if ((file->header = readHeader(file->f))) {
                return file;
            } else {
                fclose(file->f);
                free(file);
            }
        } else {
            free(file);
        }
    }
    return NULL;
}

zgdbFile* createFile(const char* filename) {
    zgdbFile* file = malloc(sizeof(zgdbFile));
    if (file) {
        file->f = fopen(filename, "w+b");
        if (file->f) {
            if ((file->header = initHeader())) {
                file->header->indexNumber += writeIndexes(file, ZGDB_DEFAULT_INDEX_CAPACITY);
                if (file->header->indexNumber == ZGDB_DEFAULT_INDEX_CAPACITY && writeHeader(file)) {
                    return file;
                } else {
                    fclose(file->f);
                    free(file->header);
                    free(file);
                }
            } else {
                fclose(file->f);
                free(file);
            }
        } else {
            free(file);
        }
    }
    return NULL;
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

sortedList* createList(zgdbFile* file) {
    sortedList* list = initList();
    if (list) {
        zgdbIndex* index = malloc(sizeof(zgdbIndex));
        if (index) {
            fseek(file->f, sizeof(zgdbHeader), SEEK_SET);
            //uint64_t pos = ftell(file->f);
            for (int i = 0; i < file->header->indexNumber; i++) {
                if (fread(index, sizeof(zgdbIndex), 1, file->f)) {
                    // += sizeof(zgdbIndex);
                    if (index->flag == INDEX_DEAD) {
                        // TODO: Продумать логику перемещения на смещение блока и возврат обратно.
                    } else if (index->flag == INDEX_NEW) {
                        listNode* node = createNode(0, i);
                        if (node) {
                            insertNode(list, node);
                        }
                    }
                } else {
                    free(index);
                    free(list);
                    return NULL;
                }
            }
            free(index);
        } else {
            free(list);
            return NULL;
        }
    }
    return list;
}
