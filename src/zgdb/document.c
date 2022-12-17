#include <malloc.h>
#include <string.h>
#include <time.h>

#include "format.h"
#include "document.h"

documentSchema* createSchema(uint64_t capacity) {
    documentSchema* schema = malloc(sizeof(documentSchema));
    if (schema) {
        schema->elements = malloc(sizeof(element) * capacity);
        if (schema->elements) {
            schema->elementCount = 0;
            schema->capacity = capacity;
        }
    }
    return schema;
}

void destroySchema(documentSchema* schema) {
    if (schema) {
        if (schema->elements) {
            free(schema->elements);
        }
        free(schema);
    }
}

bool addElementToSchema(documentSchema* schema, element el, const char* key) {
    if (schema->elementCount == schema->capacity) {
        element* newElements = malloc(sizeof(element) * (schema->capacity + 1));
        if (!newElements) {
            return false;
        }
        memcpy(newElements, schema->elements, sizeof(element) * (schema->capacity++));
        free(schema->elements);
        schema->elements = newElements;
    }
    memset(el.key, 0, 13);
    strcpy(el.key, key);
    schema->elements[schema->elementCount++] = el;
    return true;
}

bool addIntegerToSchema(documentSchema* schema, char* key, int32_t value) {
    return addElementToSchema(schema, (element) { .type = TYPE_INT, .integerValue = value }, key);
}

bool addDoubleToSchema(documentSchema* schema, char* key, double value) {
    return addElementToSchema(schema, (element) { .type = TYPE_DOUBLE, .doubleValue = value }, key);
}

bool addBooleanToSchema(documentSchema* schema, char* key, uint8_t value) {
    return addElementToSchema(schema, (element) { .type = TYPE_BOOLEAN, .booleanValue = value }, key);
}

bool addStringToSchema(documentSchema* schema, char* key, char* value) {
    return addElementToSchema(schema,
                              (element) { .type = TYPE_STRING, .stringValue = (str) { strlen(value) + 1, value }}, key);
}

bool addDocumentToSchema(documentSchema* schema, char* key, uint64_t value) {
    return addElementToSchema(schema, (element) { .type = TYPE_EMBEDDED_DOCUMENT, .documentValue = value }, key);
}

bool writeGapSize(zgdbFile* file, int64_t size) {
    // ВНИМАНИЕ: предполагается, что в момент вызова функции, мы уже находимся на нужной позиции в файле, и fseek делать не надо!
    uint8_t numberOfBytes = 0;
    /* Отметаем вариант с size = 1, поскольку в этом случае, скажем так, единственный байт дырки будет говорить сам о себе,
     * и, соответственно, будет достаточно второго fwrite. */
    if (size != 1) {
        if (size <= UINT8_MAX) {
            numberOfBytes = 1;
        } else if (size <= UINT16_MAX) {
            numberOfBytes = 2;
        } else if (size <= UINT32_MAX) {
            numberOfBytes = 4;
        } else {
            numberOfBytes = 5;
        }
        return fwrite(&numberOfBytes, sizeof(uint8_t), 1, file->f) &&
               fwrite(&size, sizeof(uint8_t), numberOfBytes, file->f);
    }
    return fwrite(&numberOfBytes, sizeof(uint8_t), 1, file->f);
}

// TODO: перенести в format.c
bool moveData(zgdbFile* file, int64_t* oldPos, int64_t* newPos, uint64_t size) {
    while (size) {
        // Определяем размер буфера и аллоцируем его:
        int64_t bufSize;
        if (size > DOCUMENT_BUF_SIZE) {
            bufSize = DOCUMENT_BUF_SIZE;
        } else {
            bufSize = (int64_t) size;
        }
        size -= bufSize;
        uint8_t* buf = malloc(bufSize);

        // Перемещаемся на прошлый адрес и заполняем буфер:
        fseeko64(file->f, *oldPos, SEEK_SET);
        if (!fread(buf, bufSize, 1, file->f)) {
            free(buf);
            return false;
        }
        *oldPos += bufSize;

        // Перемещаемся на новый адрес и пишем из буфера:
        fseeko64(file->f, *newPos, SEEK_SET);
        if (!fwrite(buf, bufSize, 1, file->f)) {
            free(buf);
            return false;
        }
        *newPos += bufSize;
        free(buf);
    }
    return true;
}

bool moveFirstDocuments(zgdbFile* file) {
    // Смещаемся к началу документов:
    int64_t oldPos = (int64_t) (sizeof(zgdbHeader) + sizeof(zgdbIndex) * file->header.indexCount +
                                file->header.firstDocumentOffset);
    fseeko64(file->f, oldPos, SEEK_SET);

    /* Перемещаем документы, пока места недостаточно. Изначально доступно file->header.firstDocumentOffset, поскольку
     * перед документами могут быть неиспользуемые байты. */
    int64_t neededSpace = sizeof(zgdbIndex) * ZGDB_DEFAULT_INDEX_CAPACITY;
    int64_t availableSpace = file->header.firstDocumentOffset;
    while (availableSpace < neededSpace) {
        // Считываем метку:
        documentHeader header;
        if (!fread(&header.mark, sizeof(uint8_t), 1, file->f)) {
            return false;
        }

        // Определяем, дырка у нас или документ. Если документ, то перемещаем его:
        uint64_t tmp = 0;
        if (header.mark == DOCUMENT_START_MARK) {
            if (!fread(&tmp, sizeof(uint8_t), 5, file->f)) {
                return false;
            }
            header.size = tmp;
            if (!fread(&tmp, sizeof(uint8_t), 5, file->f)) {
                return false;
            }
            header.indexNumber = tmp;

            /* Если есть подходящая дырка, в которую можно переместить документ, то нужно:
             * 1. Сохранить разницу между размером дырки и документа
             * 2. Забрать node из списка
             * 3. Изменить флаг соответствующего node'у индекса в файле на INDEX_NEW
             * 4. Добавить в список новый node с размером 0 и смещением, как у предыдущего node'а
             * 5. Переместиться на смещение из node'а.
             * Если подходящих дырок нет (или список пустой), то нужно переместиться в конец файла. */
            int64_t diff = 0;
            if (file->list.front && file->list.front->size >= header.size) {
                diff = (int64_t) file->list.front->size - (int64_t) header.size;
                listNode* node = popFront(&file->list);
                updateIndex(file, node->indexNumber, wrap_uint8_t(INDEX_NEW), not_present_int64_t());
                insertNode(&file->list, createNode(0, node->indexNumber));
                fseeko64(file->f, getIndex(file, node->indexNumber).offset, SEEK_SET);
                free(node);
            } else {
                fseeko64(file->f, 0, SEEK_END);
            }
            int64_t newPos = ftello64(file->f);
            updateIndex(file, header.indexNumber, not_present_uint8_t(),
                        wrap_int64_t(newPos)); // обновляем offset в индексе переносимого документа
            moveData(file, &oldPos, &newPos, header.size); // перемещаем документ

            /* Если найденная дырка была больше, чем документ, то, после переноса документа, создаём на его месте INDEX_DEAD
             * индекс для оставшейся части дырки: */
            if (diff) {
                insertNode(&file->list, createNode(diff, file->header.indexCount++));
                writeGapSize(file, diff);
                writeHeader(file);
                updateIndex(file, file->header.indexCount - 1, wrap_uint8_t(INDEX_DEAD), wrap_int64_t(newPos));
                availableSpace -= sizeof(zgdbIndex);
            }
        } else {
            if (header.mark) {
                if (!fread(&tmp, sizeof(uint8_t), header.mark, file->f)) {
                    return false;
                }
                header.size = tmp;
            } else {
                header.size = 1;
            }
            oldPos += (int64_t) header.size;
        }
        availableSpace += (int64_t) header.size; // возможно переполнение, если ZGDB_DEFAULT_INDEX_CAPACITY будет слишком большим!
        fseeko64(file->f, oldPos, SEEK_SET); // перед следующей итерацией нужно вернуться к началу
    }

    writeNewIndexes(file, availableSpace / sizeof(zgdbIndex)); // записываем новые индексы
    file->header.firstDocumentOffset = availableSpace % sizeof(zgdbIndex); // сохраняем остаток места
    writeHeader(file);
    return true;
}

uint64_t calcDocumentSize(element* elements, uint64_t elementCount) {
    uint64_t size = sizeof(documentHeader);
    for (uint64_t i = 0; i < elementCount; i++) {
        size += sizeof(uint8_t) + 13 * sizeof(char); // type и key
        element el = elements[i];
        switch (el.type) {
            case TYPE_INT:
                size += sizeof(int32_t);
                break;
            case TYPE_DOUBLE:
                size += sizeof(double);
                break;
            case TYPE_BOOLEAN:
                size += sizeof(uint8_t);
                break;
            case TYPE_STRING:
                size += sizeof(uint32_t); // размер строки
                size += sizeof(char) * el.stringValue.size; // сама строка
                break;
            case TYPE_EMBEDDED_DOCUMENT:
                size += 5; // uint64_t : 40 == 5 байт
                break;
        }
    }
    return size;
}

uint64_t writeElement(zgdbFile* file, element* el, uint64_t parentIndexNumber) {
    uint64_t bytesWritten = 0;
    bytesWritten += fwrite(&el->type, sizeof(uint8_t), 1, file->f);
    bytesWritten += fwrite(el->key, sizeof(char), 13, file->f) * sizeof(char);
    uint64_t tmp; // для битового поля (documentValue)
    switch (el->type) {
        case TYPE_INT:
            bytesWritten += fwrite(&el->integerValue, sizeof(int32_t), 1, file->f) * sizeof(int32_t);
            break;
        case TYPE_DOUBLE:
            bytesWritten += fwrite(&el->doubleValue, sizeof(double), 1, file->f) * sizeof(double);
            break;
        case TYPE_BOOLEAN:
            bytesWritten += fwrite(&el->booleanValue, sizeof(uint8_t), 1, file->f);
            break;
        case TYPE_STRING:
            bytesWritten += fwrite(&el->stringValue.size, sizeof(uint32_t), 1, file->f) * sizeof(uint32_t);
            bytesWritten += fwrite(el->stringValue.data, sizeof(char), el->stringValue.size, file->f);
            break;
        case TYPE_EMBEDDED_DOCUMENT:
            tmp = el->documentValue;
            bytesWritten += fwrite(&tmp, 5, 1, file->f) * 5; // uint64_t : 40 == 5 байт

            // Записываем в хедер вложенного документа информацию об этом (создаваемом) документе:
            int64_t pos = ftello64(file->f);
            zgdbIndex index = getIndex(file, el->documentValue);
            if (index.flag != INDEX_ALIVE) {
                return 0;
            } else {
                // Спускаемся к хедеру и пропускаем метку, размер, номер индекса. Записываем parentIndexNumber:
                fseeko64(file->f, index.offset + 11, SEEK_SET);
                if (!fwrite(&parentIndexNumber, 5, 1, file->f)) {
                    return 0;
                }
            }
            fseeko64(file->f, pos, SEEK_SET);
            break;
    }
    return bytesWritten;
}

bool writeDocument(zgdbFile* file, documentSchema* schema) {
    documentHeader header;
    header.size = calcDocumentSize(schema->elements, schema->elementCount);
    header.mark = DOCUMENT_START_MARK;
    header.parentIndexNumber = DOCUMENT_HAS_NO_PARENT; // указывает на то, что родителя нет

    if (!file->list.front) {
        moveFirstDocuments(file); // сразу выделяем индексы, если список пустой
    }

    int64_t diff = (int64_t) file->list.front->size - (int64_t) header.size;
    if (diff >= 0) {
        zgdbIndex index = getIndex(file, file->list.front->indexNumber);
        header.id.offset = index.offset;
        header.indexNumber = file->list.front->indexNumber;
        updateIndex(file, header.indexNumber, wrap_uint8_t(INDEX_ALIVE), not_present_int64_t());
        popFront(&file->list);

        if (diff) {
            if (!file->list.back || file->list.back->size) {
                moveFirstDocuments(file); // если нет хвоста или хвост - INDEX_DEAD, то выделяем новые индексы
            }
            listNode* node = popBack(&file->list);
            updateIndex(file, node->indexNumber, wrap_uint8_t(INDEX_DEAD),
                        wrap_int64_t(index.offset + (int64_t) header.size));
            node->size = diff;
            insertNode(&file->list, node);
        }

        fseeko64(file->f, header.id.offset, SEEK_SET);
    } else {
        // В любом случае будем писать в конец файла, но возможно, что нет INDEX_NEW индексов, поэтому выделяем новые
        if (file->list.back->size != 0) {
            moveFirstDocuments(file);
        }
        fseeko64(file->f, 0, SEEK_END);
        header.id.offset = ftello64(file->f);
        header.indexNumber = file->list.back->indexNumber;
        updateIndex(file, header.indexNumber, wrap_uint8_t(INDEX_ALIVE), wrap_int64_t(header.id.offset));
        popBack(&file->list);
    }

    header.id.timestamp = (uint32_t) time(NULL);
    uint64_t bytesWritten = fwrite(&header, sizeof(documentHeader), 1, file->f) * sizeof(documentHeader);
    for (uint64_t i = 0; i < schema->elementCount; i++) {
        bytesWritten += writeElement(file, schema->elements + i, header.indexNumber);
    }
    if (diff > 0) {
        if (!writeGapSize(file, diff)) {
            return false;
        }
    }
    return bytesWritten == header.size;
}

bool removeDocument(zgdbFile* file, uint64_t i) {
    zgdbIndex index = getIndex(file, i);
    if (index.flag == INDEX_ALIVE) {
        // Считываем хедер документа:
        fseeko64(file->f, index.offset, SEEK_SET);
        documentHeader header;
        if (!fread(&header, sizeof(documentHeader), 1, file->f)) {
            return false;
        }

        // Записываем вместо хедера документа "хедер дырки", изменяем флаг индекса документа и добавляем дырку в список:
        fseeko64(file->f, index.offset, SEEK_SET);
        if (!writeGapSize(file, (int64_t) header.size) ||
            !updateIndex(file, i, wrap_uint8_t(INDEX_DEAD), not_present_int64_t())) {
            return false;
        }
        insertNode(&file->list, createNode(header.size, i));

        // Ищем детей и удаляем в них информацию о родителе:
        for (uint64_t j = 0; j < file->header.indexCount; j++) {
            if (j != i) {
                index = getIndex(file, j);
                if (index.flag == INDEX_ALIVE) {
                    // Считываем хедер:
                    fseeko64(file->f, index.offset, SEEK_SET);
                    if (!fread(&header, sizeof(documentHeader), 1, file->f)) {
                        return false;
                    }
                    // Если удаляемый документ - родитель считанного, то перезаписываем хедер:
                    if (header.parentIndexNumber == i) {
                        header.parentIndexNumber = DOCUMENT_HAS_NO_PARENT;
                        fseeko64(file->f, index.offset, SEEK_SET);
                        if (!fwrite(&header, sizeof(documentHeader), 1, file->f)) {
                            return false;
                        }
                    }
                } else if (index.flag == INDEX_NOT_EXIST) {
                    return false;
                }
            }
        }
    } else if (index.flag == INDEX_NOT_EXIST) {
        return false;
    }
    return true;
}

element readElement(zgdbFile* file, char* neededKey, uint64_t i) {
    zgdbIndex index = getIndex(file, i);
    if (index.flag == INDEX_ALIVE) {
        bool matchFound = false;
        documentHeader header;
        fseeko64(file->f, index.offset, SEEK_SET); // спуск в документ по смещению
        uint64_t bytesRead = fread(&header, sizeof(documentHeader), 1, file->f) * sizeof(documentHeader);
        element el;
        while (!matchFound && bytesRead < header.size) {
            bytesRead += fread(&el.type, sizeof(uint8_t), 1, file->f);
            bytesRead += fread(&el.key, sizeof(char), 13, file->f) * sizeof(char);
            uint64_t tmp; // для битового поля (documentValue)
            matchFound = strcmp(el.key, neededKey) == 0;
            switch (el.type) {
                case TYPE_INT:
                    bytesRead += fread(&el.integerValue, sizeof(int32_t), 1, file->f) * sizeof(int32_t);
                    break;
                case TYPE_DOUBLE:
                    bytesRead += fread(&el.doubleValue, sizeof(double), 1, file->f) * sizeof(double);
                    break;
                case TYPE_BOOLEAN:
                    bytesRead += fread(&el.booleanValue, sizeof(uint8_t), 1, file->f);
                    break;
                case TYPE_STRING:
                    if (matchFound) {
                        bytesRead += fread(&el.stringValue.size, sizeof(uint32_t), 1, file->f) * sizeof(uint32_t);
                        el.stringValue.data = malloc(sizeof(char) * el.stringValue.size);
                        bytesRead += fread(&el.stringValue.data, sizeof(char), el.stringValue.size, file->f);
                    } else {
                        bytesRead += fread(&el.integerValue, sizeof(uint32_t), 1, file->f) * sizeof(uint32_t);
                        fseeko64(file->f, el.integerValue, SEEK_CUR);
                        bytesRead += el.integerValue;
                    }
                    break;
                case TYPE_EMBEDDED_DOCUMENT:
                    bytesRead = fread(&tmp, 5, 1, file->f) * 5; // uint64_t : 40 == 5 байт
                    el.documentValue = tmp;
                    break;
            }
        }
        if (matchFound) {
            return el;
        }
    }
    return (element) { 0 };
}

void destroyElement(element el) {
    if (el.type == TYPE_STRING) {
        free(el.stringValue.data);
    }
}