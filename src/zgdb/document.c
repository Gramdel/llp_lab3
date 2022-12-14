#include <malloc.h>
#include <string.h>
#include <time.h>
#include "document.h"

documentSchema* createSchema(size_t capacity) {
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
            for (int i = 0; i < schema->elementCount; i++) {
                if (schema->elements[i].type == TYPE_STRING && schema->elements[i].stringValue) {
                    if (schema->elements[i].stringValue->data) {
                        free(schema->elements[i].stringValue->data);
                    }
                    free(schema->elements[i].stringValue);
                }
            }
            free(schema->elements);
        }
        free(schema);
    }
}
// TODO: может как-то упростить? switch?
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
    uint32_t size = strlen(value);
    str* s = malloc(sizeof(str));
    if (s) {
        s->size = size;
        s->data = value;
        return addElementToSchema(schema, (element) { .type = TYPE_STRING, .stringValue = s }, key);
    }
    return false;
}

bool addDocumentToSchema(documentSchema* schema, char* key, uint64_t value) {
    return addElementToSchema(schema, (element) { .type = TYPE_EMBEDDED_DOCUMENT, .documentValue = value }, key);
}

uint64_t calcDocumentSize(element* elements, size_t elementNumber) {
    uint64_t size = sizeof(documentHeader);
    for (int i = 0; i < elementNumber; i++) {
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
                size += sizeof(char) * el.stringValue->size;
                break;
            case TYPE_EMBEDDED_DOCUMENT:
                size += 5; // uint64_t : 40 == 5 байт
                break;
        }
    }
    return size;
}

// TODO: может возвращать void? Или нужны проверки
bool moveFirstDocuments(zgdbFile* file, sortedList* list) {
    // Смещаемся к началу документов:
    fseeko64(file->f, (int64_t) (sizeof(zgdbHeader) + sizeof(zgdbIndex) * file->header->indexCount +
                                 file->header->firstDocumentOffset), SEEK_SET);
    int64_t oldPos = ftello64(file->f); // сохраняем позицию, с которой начинается первый документ
    int64_t neededSpace = sizeof(zgdbIndex) * ZGDB_DEFAULT_INDEX_CAPACITY;
    int64_t availableSpace = file->header->firstDocumentOffset; // перед первым документом могут быть неиспользуемые байты
    do {
        documentHeader header;
        fread(&header, sizeof(documentHeader), 1, file->f);
        oldPos += sizeof(documentHeader);

        /* Если есть подходящая дырка, делаем следующие операции:
         * 1. Сохраняем разницу между размерами дырки и документа
         * 2. Забираем node из списка
         * 3. Добавляем node обратно, изменив size на 0
         * 4. Делаем соответствующий индекс в файле INDEX_NEW
         * 5. Сохраняем из node смещение (fseek + ftell)
         * Таким образом, INDEX_DEAD превращается в INDEX_NEW, а его бывший offset будет прикреплен к индексу
         * переносимого документа.
         * Если подходящих дырок нет (или список пустой), то перемещаемся в конец файла, смещение также сохраняем. */
        uint64_t diff = 0;
        if (list->front && list->front->size >= header.size) {
            diff = list->front->size - header.size;
            listNode* node = popFront(list);
            node->size = 0;
            insertNode(list, node);
            updateIndex(file, node->indexNumber, wrap_uint8_t(INDEX_NEW), not_present_int64_t());

            zgdbIndex index = getIndex(file, node->indexNumber);
            fseeko64(file->f, index.offset, SEEK_SET);
            free(node);
        } else {
            fseeko64(file->f, 0, SEEK_END);
        }
        int64_t newPos = ftello64(file->f);
        updateIndex(file, header.indexNumber, not_present_uint8_t(),
                    wrap_int64_t(newPos)); // обновляем offset в индексе переносимого документа

        // Перемещаем документ:
        fwrite(&header, sizeof(documentHeader), 1, file->f);
        newPos += sizeof(documentHeader);
        uint64_t bytesLeft = header.size - sizeof(documentHeader);
        do {
            int64_t bufSize;
            if (bytesLeft > DOCUMENT_BUF_SIZE) {
                bufSize = DOCUMENT_BUF_SIZE;
            } else {
                bufSize = (int64_t) bytesLeft;
            }
            bytesLeft -= bufSize;

            uint8_t* buf = malloc(bufSize);
            fseeko64(file->f, oldPos, SEEK_SET);
            fread(buf, bufSize, 1, file->f);
            oldPos += bufSize;
            fseeko64(file->f, newPos, SEEK_SET);
            fwrite(buf, bufSize, 1, file->f);
            newPos += bufSize;
            free(buf);
        } while (bytesLeft);

        /* Если найденная дырка больше, чем документ, то, после переноса документа, создаём на его месте INDEX_DEAD
         * индекс для оставшейся части дырки */
        if (diff) {
            insertNode(list, createNode(diff, file->header->indexCount++));
            writeHeader(file);
            updateIndex(file, file->header->indexCount - 1, wrap_uint8_t(INDEX_DEAD), wrap_int64_t(newPos));
            availableSpace -= sizeof(zgdbIndex);
        }

        fseeko64(file->f, oldPos, SEEK_SET); // перед следующей итерацией нужно вернуться к началу
        availableSpace += (int64_t) header.size; // возможно переполнение, если ZGDB_DEFAULT_INDEX_CAPACITY будет слишком большим!
    } while (availableSpace < neededSpace);

    writeIndexes(file, availableSpace / sizeof(zgdbIndex), list);
    file->header->firstDocumentOffset = availableSpace % sizeof(zgdbIndex); // сохраняем остаток места
    writeHeader(file);
    return true;
}

uint64_t writeElement(zgdbFile* file, element* el) {
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
            bytesWritten += fwrite(el->stringValue->data, sizeof(char), el->stringValue->size, file->f);
            break;
        case TYPE_EMBEDDED_DOCUMENT:
            // TODO: может здесь записывать в хедеры детей инфу?
            tmp = el->documentValue;
            bytesWritten += fwrite(&tmp, 5, 1, file->f) * 5; // uint64_t : 40 == 5 байт
            break;
    }
    return bytesWritten;
}

bool writeDocument(zgdbFile* file, sortedList* list, documentSchema* schema) {
    documentHeader header;
    header.size = calcDocumentSize(schema->elements, schema->elementCount);
    header.parentIndexNumber = DOCUMENT_HAS_NO_PARENT; // указывает на то, что родителя нет

    if (!list->front) {
        moveFirstDocuments(file, list); // сразу выделяем индексы, если список пустой
    }

    int64_t diff = (int64_t) list->front->size - (int64_t) header.size;
    if (diff >= 0) {
        zgdbIndex index = getIndex(file, list->front->indexNumber);
        header.id.offset = index.offset;
        header.indexNumber = list->front->indexNumber;
        updateIndex(file, header.indexNumber, wrap_uint8_t(INDEX_ALIVE), not_present_int64_t());
        popFront(list);

        if (diff) {
            if (!list->back || list->back->size) {
                moveFirstDocuments(file, list); // если нет хвоста или хвост - INDEX_DEAD, то выделяем новые индексы
            }
            listNode* node = popBack(list);
            updateIndex(file, node->indexNumber, wrap_uint8_t(INDEX_DEAD),
                        wrap_int64_t(index.offset + (int64_t) header.size));
            node->size = diff;
            insertNode(list, node);
        }

        fseeko64(file->f, header.id.offset, SEEK_SET);
    } else {
        // В любом случае будем писать в конец файла, но возможно, что нет INDEX_NEW индексов, поэтому выделяем новые
        if (list->back->size != 0) {
            moveFirstDocuments(file, list);
        }
        header.id.offset = ftello64(file->f);
        header.indexNumber = list->back->indexNumber;
        updateIndex(file, header.indexNumber, wrap_uint8_t(INDEX_ALIVE), wrap_int64_t(header.id.offset));
        popBack(list);
        fseeko64(file->f, 0, SEEK_END);
    }

    header.id.timestamp = (uint32_t) time(NULL);
    uint64_t bytesWritten = fwrite(&header, sizeof(documentHeader), 1, file->f) * sizeof(documentHeader);
    for (int i = 0; i < schema->elementCount; i++) {
        bytesWritten += writeElement(file, schema->elements + i);
    }
    return bytesWritten == header.size;
}

bool removeDocument(zgdbFile* file, sortedList* list, uint64_t i) {
    zgdbIndex index = getIndex(file, i);
    if (index.flag != INDEX_NOT_EXIST) {
        updateIndex(file, i, wrap_uint8_t(INDEX_DEAD), not_present_int64_t());
        fseeko64(file->f, index.offset, SEEK_SET);
        uint64_t size;
        fread(&size, 5, 1, file->f); // uint64_t : 40 == 5 байт
        insertNode(list, createNode(size, i));

        // Ищем детей и удаляем в них информацию о родителе:
        for (int j = 0; j < file->header->indexCount; j++) {
            if (j != i) {
                index = getIndex(file, j);
                if (index.flag == INDEX_ALIVE) {
                    documentHeader header;
                    fseeko64(file->f, index.offset, SEEK_SET);
                    fread(&header, sizeof(documentHeader), 1, file->f);
                    if (header.parentIndexNumber == i) {
                        header.parentIndexNumber = DOCUMENT_HAS_NO_PARENT;
                        fseeko64(file->f, index.offset, SEEK_SET);
                        fwrite(&header, sizeof(documentHeader), 1, file->f);
                    }
                }
            }
        }
        return true;
    }
    return false;
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
                        el.stringValue = malloc(sizeof(str));
                        bytesRead += fread(&el.stringValue->size, sizeof(uint32_t), 1, file->f) * sizeof(uint32_t);
                        el.stringValue->data = malloc(sizeof(char) * (el.stringValue->size + 1));
                        bytesRead += fread(&el.stringValue->data, sizeof(char), el.stringValue->size + 1, file->f);
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