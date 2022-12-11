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
        header->firstDocumentOffset = 0;
        header->indexCount = 0;
    }
    return header;
}

bool writeHeader(zgdbFile* file) {
    rewind(file->f);
    return fwrite(file->header, sizeof(zgdbHeader), 1, file->f);
}

size_t writeIndexes(zgdbFile* file, size_t count, sortedList* list) {
    zgdbIndex index = { INDEX_NEW, 0 };
    fseek(file->f, sizeof(zgdbHeader), SEEK_SET);
    for (int i = 0; i < file->header->indexCount; i++) {
        fseek(file->f, sizeof(zgdbIndex), SEEK_CUR);
    }
    size_t written = 0;
    for (int i = 0; i < count; i++) {
        written += fwrite(&index, sizeof(zgdbIndex), 1, file->f);
        insertNode(list, createNode(0, file->header->indexCount++));
    }
    return written;
}

zgdbIndex* getIndex(zgdbFile* file, uint64_t i) {
    if (i < file->header->indexCount) {
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
    return NULL;
}

bool updateIndex(zgdbFile* file, uint64_t i, opt_uint8_t flag, opt_int64_t offset) {
    int64_t pos = ftello64(file->f);
    fseek(file->f, sizeof(zgdbHeader), SEEK_SET);
    for (int j = 0; j < i; j++) {
        fseek(file->f, sizeof(zgdbIndex), SEEK_CUR);
    }
    size_t written = 0;
    if (flag.isPresent) {
        written += fwrite(&flag.value, sizeof(uint8_t), 1, file->f);
    } else {
        fseek(file->f, sizeof(uint8_t), SEEK_CUR);
    }
    if (offset.isPresent) {
        written += fwrite(&offset.value, sizeof(int64_t), 1, file->f);
    }
    fseeko64(file->f, pos, SEEK_SET);
    return (flag.isPresent == offset.isPresent) ? (written == 2) : (written == 1); // XOR
}

// TODO: добавить внутрь загрузку списка индексов
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

zgdbFile* createFile(const char* filename, sortedList* list) {
    zgdbFile* file = malloc(sizeof(zgdbFile));
    if (file) {
        file->f = fopen(filename, "w+b");
        if (file->f) {
            if ((file->header = initHeader())) {
                writeIndexes(file, ZGDB_DEFAULT_INDEX_CAPACITY, list);
                if (file->header->indexCount == ZGDB_DEFAULT_INDEX_CAPACITY && writeHeader(file)) {
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

// TODO: подчистить функцию
sortedList* createList(zgdbFile* file) {
    sortedList* list = initList();
    if (list) {
        zgdbIndex* index = malloc(sizeof(zgdbIndex));
        if (index) {
            fseek(file->f, sizeof(zgdbHeader), SEEK_SET);
            int64_t offset = sizeof(zgdbHeader);
            for (int i = 0; i < file->header->indexCount; i++) {
                if (fread(index, sizeof(zgdbIndex), 1, file->f)) {
                    offset += sizeof(zgdbIndex);
                    if (index->flag == INDEX_DEAD) {
                        // TODO: проверить, что это работает
                        uint64_t size;
                        fseeko64(file->f, index->offset, SEEK_CUR);
                        if (fread(&size, 5, 1, file->f)) {
                            listNode* node = createNode(size, i);
                            if (node) {
                                insertNode(list, node);
                            }
                            fseeko64(file->f, offset, SEEK_SET);
                        } else {
                            free(index);
                            free(list);
                            return NULL;
                        }
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
