#include <malloc.h>

#include "format.h"
#include "document.h"

bool writeHeader(zgdbFile* file) {
    rewind(file->f);
    return fwrite(&file->header, sizeof(zgdbHeader), 1, file->f);
}

bool writeNewIndexes(zgdbFile* file, uint64_t count) {
    zgdbIndex index = { INDEX_NEW, 0 };
    fseeko64(file->f, (int64_t) (sizeof(zgdbHeader) + sizeof(zgdbIndex) * file->header.indexCount), SEEK_SET);
    uint64_t written = 0;
    for (uint64_t i = 0; i < count; i++) {
        written += fwrite(&index, sizeof(zgdbIndex), 1, file->f);
        insertNode(&file->list, createNode(0, file->header.indexCount++));
    }
    return written == count;
}

zgdbIndex getIndex(zgdbFile* file, uint64_t i) {
    zgdbIndex index = { INDEX_NOT_EXIST, 0 };
    if (i < file->header.indexCount) {
        fseeko64(file->f, (int64_t) (sizeof(zgdbHeader) + sizeof(zgdbIndex) * i), SEEK_SET);
        fread(&index, sizeof(zgdbIndex), 1, file->f);
    }
    return index;
}

bool updateIndex(zgdbFile* file, uint64_t i, opt_uint8_t flag, opt_int64_t offset) {
    if (i < file->header.indexCount) {
        int64_t pos = ftello64(file->f);
        fseeko64(file->f, (int64_t) (sizeof(zgdbHeader) + sizeof(zgdbIndex) * i), SEEK_SET);
        uint64_t written = 0;
        if (flag.isPresent) {
            written += fwrite(&flag.value, sizeof(uint8_t), 1, file->f);
        } else {
            fseeko64(file->f, sizeof(uint8_t), SEEK_CUR);
        }
        if (offset.isPresent) {
            written += fwrite(&offset.value, sizeof(int64_t), 1, file->f);
        }
        fseeko64(file->f, pos, SEEK_SET);
        return (flag.isPresent == offset.isPresent) ? (written == 2) : (written == 1); // XOR
    }
    return false;
}

bool loadList(zgdbFile* file) {
    // ВНИМАНИЕ: предполагается, что в момент вызова функции хедер уже загружен, и fseek делать не надо
    int64_t pos = sizeof(zgdbHeader);
    for (uint64_t i = 0; i < file->header.indexCount; i++) {
        pos += sizeof(zgdbIndex);
        zgdbIndex index;
        if (fread(&index, sizeof(zgdbIndex), 1, file->f)) {
            if (index.flag != INDEX_ALIVE) {
                documentHeader header = { 0 };
                if (index.flag == INDEX_DEAD) {
                    fseeko64(file->f, index.offset, SEEK_SET);
                    if (!fread(&header.mark, sizeof(uint8_t), 1, file->f)) {
                        return false;
                    }
                    // Определяем по метке, сколько байт надо считать в size (и нужно ли вообще):
                    if (header.mark) {
                        if (header.mark == DOCUMENT_START_MARK) {
                            header.mark = 5;
                        }
                        uint64_t tmp = 0;
                        if (!fread(&tmp, sizeof(uint8_t), header.mark, file->f)) {
                            return false;
                        }
                        header.size = tmp;
                    } else {
                        header.size = 1;
                    }
                    fseeko64(file->f, pos, SEEK_SET);
                }
                insertNode(&file->list, createNode(header.size, i)); // TODO: обернуть в if?
            }
        } else {
            return false;
        }
    }
    return true;
}

zgdbFile* loadFile(const char* filename) {
    zgdbFile* file = malloc(sizeof(zgdbFile));
    if (file) {
        file->f = fopen(filename, "r+b");
        file->list = (sortedList) { NULL, NULL };
        if (file->f && fread(&file->header, sizeof(zgdbHeader), 1, file->f) && loadList(file)) {
            return file;
        }
    }
    closeFile(file);
    return NULL;
}

zgdbFile* createFile(const char* filename) {
    zgdbFile* file = malloc(sizeof(zgdbFile));
    if (file) {
        file->f = fopen(filename, "w+b");
        file->header = (zgdbHeader) { ZGDB_FILETYPE, 0, 0 };
        file->list = (sortedList) { NULL, NULL };
        // ВНИМАНИЕ: заголовок записывается после индексов, поскольку writeNewIndexes изменяет indexCount в заголовке.
        if (file->f && writeNewIndexes(file, ZGDB_DEFAULT_INDEX_CAPACITY) && writeHeader(file)) {
            return file;
        }
    }
    closeFile(file);
    return NULL;
}

void closeFile(zgdbFile* file) {
    if (file) {
        if (file->f) {
            fclose(file->f);
        }
        destroyList(&file->list);
        free(file);
    }
}