#include <malloc.h>
#include <time.h>
#include <string.h>

#include "document.h"
#include "element.h"
#include "query.h"

bool moveFirstDocuments(zgdbFile* file) {
    // Смещаемся к началу документов:
    int64_t newPos;
    int64_t oldPos = (int64_t) (sizeof(zgdbHeader) + sizeof(zgdbIndex) * file->header.indexCount +
                                file->header.firstDocumentOffset);
    /* Перемещаем документы, пока места недостаточно. Изначально доступно file->header.firstDocumentOffset, поскольку
     * перед документами могут быть неиспользуемые байты: */
    int64_t neededSpace = sizeof(zgdbIndex) * ZGDB_DEFAULT_INDEX_CAPACITY;
    int64_t availableSpace = file->header.firstDocumentOffset;
    while (availableSpace < neededSpace) {
        // Считываем заголовок документа:
        fseeko64(file->f, oldPos, SEEK_SET);
        documentHeader header;
        if (!fread(&header, sizeof(documentHeader), 1, file->f)) {
            return false;
        }
        // Считываем индекс, привязанный к документу:
        uint64_t newHeaderSize = 0;
        zgdbIndex index = getIndex(file, header.indexNumber);
        if (index.flag == INDEX_DEAD) {
            // Если наш документ - дырка, удаляем его из списка индексов и делаем INDEX_NEW.
            if (!removeNodeByIndexNumber(&file->list, header.indexNumber) ||
                !updateIndex(file, header.indexNumber, wrap_uint8_t(INDEX_NEW), wrap_int64_t(0))) {
                return false;
            }
            insertNode(&file->list, createNode(0, header.indexNumber));
            oldPos += (int64_t) header.size;
        } else if (index.flag == INDEX_ALIVE) {
            /* Если документ живой, то его нужно переместить.
             * Если есть подходящая дырка, в которую можно переместить документ, то нужно сделать индекс дырки новым
             * (flag = INDEX_NEW, offset = 0), а прошлое смещение дырки записать в индекс переносимого блока.
             * Если подходящих дырок нет (или список пустой), то нужно перемещать документ в конец файла. */
            if (file->list.front && file->list.front->size >= header.size) {
                zgdbIndex gapIndex = getIndex(file, file->list.front->indexNumber);
                if (gapIndex.flag != INDEX_DEAD ||
                    !updateIndex(file, file->list.front->indexNumber, wrap_uint8_t(INDEX_NEW), wrap_int64_t(0))) {
                    return false;
                }
                newPos = gapIndex.offset;
                newHeaderSize = file->list.front->size;
                // Записываем дырку обратно в список, но уже с размером 0:
                listNode* node = popFront(&file->list);
                node->size = 0;
                insertNode(&file->list, node);
                // Если дырка больше, чем надо, записываем TYPE_NOT_EXIST в том месте, где будет заканчиваться документ:
                if (newHeaderSize > header.size) {
                    uint8_t startOfUnusedSpaceMark = TYPE_NOT_EXIST;
                    fseeko64(file->f, newPos + (int64_t) header.size, SEEK_SET);
                    if (!fwrite(&startOfUnusedSpaceMark, sizeof(uint8_t), 1, file->f)) {
                        return false;
                    }
                }
            } else {
                newPos = file->header.fileSize;
                // Обновляем размер файла:
                file->header.fileSize += (int64_t) header.size;
                if (!writeHeader(file)) {
                    return false;
                }
            }
            // Перемещаем документ, обновляем смещение в его индексе:
            if (!updateIndex(file, header.indexNumber, not_present_uint8_t(), wrap_int64_t(newPos)) ||
                !moveData(file, &oldPos, &newPos, header.size)) {
                return false;
            }
        } else {
            return false;
        }
        availableSpace += (int64_t) header.size; // возможно переполнение, если ZGDB_DEFAULT_INDEX_CAPACITY будет слишком большим!
        // Смещаемся к началу нового места документа и обновляем его заголовок, если он был перемещён в дырку:
        if (newHeaderSize) {
            fseeko64(file->f, newPos - (int64_t) header.size, SEEK_SET);
            header.size = newHeaderSize;
            if (!fwrite(&header, sizeof(documentHeader), 1, file->f)) {
                return false;
            }
        }
    }
    // Записываем новые индексы и сохраняем остаток места:
    file->header.firstDocumentOffset = availableSpace % sizeof(zgdbIndex);
    if (!writeNewIndexes(file, availableSpace / sizeof(zgdbIndex)) || !writeHeader(file)) {
        return false;
    }
    return true;
}

uint64_t calcDocumentSize(documentSchema* schema) {
    uint64_t size = sizeof(documentHeader);
    for (uint64_t i = 0; i < schema->elementCount; i++) {
        size += sizeof(uint8_t) + 13 * sizeof(char); // type и key
        element el = schema->elements[i];
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

documentRef* writeDocument(zgdbFile* file, documentSchema* schema) {
    if (!schema) {
        return false;
    }
    int64_t pos = ftello64(file->f); // сохраняем текущую позицию, чтобы вернуться в неё после записи
    documentHeader header;
    header.size = calcDocumentSize(schema);
    header.parentIndexNumber = DOCUMENT_NOT_EXIST; // указывает на то, что родителя нет

    // Сразу выделяем индексы, если список пустой:
    if (!file->list.front && !moveFirstDocuments(file)) {
        return NULL;
    }
    // Если есть подходящая дырка, то пишем документ туда:
    uint64_t newSize = 0;
    int64_t diff = (int64_t) file->list.front->size - (int64_t) header.size;
    if (diff >= 0) {
        // Считываем индекс дырки и обновляем его (делаем INDEX_ALIVE):
        zgdbIndex index = getIndex(file, file->list.front->indexNumber);
        if (index.flag != INDEX_DEAD ||
            !updateIndex(file, file->list.front->indexNumber, wrap_uint8_t(INDEX_ALIVE), not_present_int64_t())) {
            return NULL;
        }
        // Заполняем заголовок документа:
        header.indexNumber = file->list.front->indexNumber;
        header.id.offset = index.offset;
        // Если дырка больше, чем надо, записываем TYPE_NOT_EXIST в том месте, где будет заканчиваться документ:
        if (diff) {
            uint8_t startOfUnusedSpaceMark = TYPE_NOT_EXIST;
            fseeko64(file->f, index.offset + (int64_t) header.size, SEEK_SET);
            if (!fwrite(&startOfUnusedSpaceMark, sizeof(uint8_t), 1, file->f)) {
                return NULL;
            }
            newSize = file->list.front->size;
        }
        free(popFront(&file->list));
    } else {
        // В любом случае будем писать в конец файла, но, возможно, надо выделить новые индексы. Затем обновляем индекс дырки:
        if (file->list.back->size != 0 && !moveFirstDocuments(file) ||
            !updateIndex(file, file->list.back->indexNumber, wrap_uint8_t(INDEX_ALIVE),
                         wrap_int64_t(file->header.fileSize))) {
            return NULL;
        }
        // Заполняем заголовок документа:
        header.indexNumber = file->list.back->indexNumber;
        header.id.offset = file->header.fileSize;
        free(popBack(&file->list));
        // Обновляем размер файла
        file->header.fileSize += (int64_t) header.size;
        if (!writeHeader(file)) {
            return NULL;
        }
    }

    // Пропускаем заголовок и записываем сначала основную часть документа:
    uint64_t bytesLeft = header.size;
    fseeko64(file->f, header.id.offset + (int64_t) sizeof(documentHeader), SEEK_SET);
    for (uint64_t i = 0; i < schema->elementCount; i++) {
        bytesLeft -= writeElement(file, schema->elements + i, header.indexNumber);
    }
    // Обновляем размер документа (если надо) и записываем время создания документа в заголовок:
    if (newSize) {
        header.size = newSize;
    }
    header.id.timestamp = (uint32_t) time(NULL);
    // Перемещаемся к началу и записываем заголовок:
    fseeko64(file->f, header.id.offset, SEEK_SET);
    bytesLeft -= fwrite(&header, sizeof(documentHeader), 1, file->f) * sizeof(documentHeader);
    documentRef* ref = NULL;
    if (!bytesLeft) {
        ref = malloc(sizeof(documentRef));
        if (ref) {
            ref->indexNumber = header.indexNumber;
        }
    }
    fseeko64(file->f, pos, SEEK_SET);
    return ref;
}

indexFlag removeEmbeddedDocument(zgdbFile* file, uint64_t childIndexNumber, uint64_t parentIndexNumber) {
    zgdbIndex index = getIndex(file, childIndexNumber);
    if (index.flag == INDEX_ALIVE) {
        // Считываем хедер документа:
        fseeko64(file->f, index.offset, SEEK_SET);
        documentHeader header;
        if (!fread(&header, sizeof(documentHeader), 1, file->f)) {
            return INDEX_NOT_EXIST;
        }

        /* Если номер индекса документа-родителя не совпадает с header.parentIndexNumber в документе-ребёнке, и при этом
         * инициатор удаления - документ, а не пользователь (т.е. номер индекса документа-родителя отличен от DOCUMENT_NOT_EXIST),
         * то нужно отменить удаление: */
        if (parentIndexNumber != header.parentIndexNumber && parentIndexNumber != DOCUMENT_NOT_EXIST) {
            return index.flag;
        }

        // Если header.parentIndexNumber в документе-ребёнке не равен DOCUMENT_NOT_EXIST, то обновляем информацию в родителе:
        if (header.parentIndexNumber != DOCUMENT_NOT_EXIST) {
            // Обновляем
        }

        // Изменяем флаг индекса документа и добавляем дырку в список:
        if (!updateIndex(file, childIndexNumber, wrap_uint8_t(INDEX_DEAD), not_present_int64_t())) {
            return INDEX_NOT_EXIST;
        }
        insertNode(&file->list, createNode(header.size, childIndexNumber));

        // Удаляем детей документа:
        for (uint64_t i = 0; i < file->header.indexCount; i++) {
            if (i != childIndexNumber) {
                if (removeEmbeddedDocument(file, i, childIndexNumber) == INDEX_NOT_EXIST) {
                    return INDEX_NOT_EXIST;
                }
            }
        }
    }
    return index.flag;
}

bool removeDocument(zgdbFile* file, documentRef* ref) {
    bool result = false;
    if (ref && ref->indexNumber != DOCUMENT_NOT_EXIST) {
        result = removeEmbeddedDocument(file, ref->indexNumber, DOCUMENT_NOT_EXIST) != INDEX_NOT_EXIST;
        ref->indexNumber = DOCUMENT_NOT_EXIST;
    }
    return result;
}

void printEmbeddedDocument(zgdbFile* file, uint64_t i, uint64_t nestingLevel) {
    if (i == DOCUMENT_NOT_EXIST) {
        printf("%*sDocument does not exist!\n\n", nestingLevel * 2, "");
    } else {
        zgdbIndex index = getIndex(file, i);
        element* el;
        if (index.flag == INDEX_ALIVE) {
            el = malloc(sizeof(element));
            if (el) {
                fseeko64(file->f, index.offset, SEEK_SET); // спуск в документ по смещению
                documentHeader header;
                if (fread(&header, sizeof(documentHeader), 1, file->f)) {
                    printf("%*s### ID: %08X%016X SIZE: %ld ###\n", nestingLevel * 2, "", header.id.timestamp,
                           header.id.offset, header.size);
                    uint64_t bytesRead = sizeof(documentHeader);
                    uint64_t tmp = 0; // для битового поля (documentValue)
                    while (bytesRead < header.size) {
                        if (!fread(&el->type, sizeof(uint8_t), 1, file->f) ||
                            fread(&el->key, sizeof(char), 13, file->f) != 13) {
                            destroyElement(el);
                            goto exit;
                        }
                        bytesRead += sizeof(uint8_t) + sizeof(char) * 13;
                        switch (el->type) {
                            case TYPE_NOT_EXIST:
                                bytesRead = header.size;
                                break;
                            case TYPE_INT:
                                if (!fread(&el->integerValue, sizeof(int32_t), 1, file->f)) {
                                    destroyElement(el);
                                    goto exit;
                                }
                                bytesRead += sizeof(int32_t);
                                break;
                            case TYPE_DOUBLE:
                                if (!fread(&el->doubleValue, sizeof(double), 1, file->f)) {
                                    destroyElement(el);
                                    goto exit;
                                }
                                bytesRead += sizeof(double);
                                break;
                            case TYPE_BOOLEAN:
                                if (!fread(&el->booleanValue, sizeof(uint8_t), 1, file->f)) {
                                    destroyElement(el);
                                    goto exit;
                                }
                                bytesRead += sizeof(uint8_t);
                                break;
                            case TYPE_STRING:
                                if (fread(&el->stringValue.size, sizeof(uint32_t), 1, file->f)) {
                                    el->stringValue.data = malloc(sizeof(char) * el->stringValue.size);
                                    if (el->stringValue.data) {
                                        if (fread(el->stringValue.data, sizeof(char), el->stringValue.size, file->f) ==
                                            el->stringValue.size) {
                                            bytesRead += sizeof(uint32_t) + sizeof(char) * el->stringValue.size;
                                            break;
                                        }
                                        free(el->stringValue.data);
                                    }
                                }
                                destroyElement(el);
                                goto exit;
                            case TYPE_EMBEDDED_DOCUMENT:
                                el->documentValue = malloc(sizeof(documentRef));
                                if (el->documentValue) {
                                    if (fread(&tmp, 5, 1, file->f)) {
                                        el->documentValue->indexNumber = tmp;
                                        bytesRead += 5;
                                        break;
                                    }
                                    free(el->documentValue);
                                }
                                destroyElement(el);
                                goto exit;
                        }
                        printElementOfEmbeddedDocument(file, el, nestingLevel);
                    }
                    destroyElement(el);
                    return;
                }
            }
        }
        exit:
        printf("%*sAn error occurred!\n", nestingLevel * 2, "");
    }
}

void printDocument(zgdbFile* file, documentRef* ref) {
    if (ref && ref->indexNumber != DOCUMENT_NOT_EXIST) {
        printEmbeddedDocument(file, ref->indexNumber, 0);
    } else {
        printf("Document does not exist!\n");
    }
    printf("\n");
}

documentRef* getDocumentByID(zgdbFile* file, char* idAsString) {
    if (idAsString && strlen(idAsString) == 24) {
        // Разбиваем нашу idAsString на две hex-строки:
        char timestampAsString[8];
        char* offsetAsString = idAsString + 8;
        strncpy(timestampAsString, idAsString, 8);
        // Парсим:
        documentId id;
        id.timestamp = strtol(timestampAsString, NULL, 16);
        id.offset = strtol(offsetAsString, NULL, 16);
        for (uint64_t i = 0; i < file->header.indexCount; i++) {
            zgdbIndex index = getIndex(file, i);
            if (index.flag == INDEX_ALIVE) {
                documentHeader header;
                fseeko64(file->f, index.offset, SEEK_SET);
                if (!fread(&header, sizeof(documentHeader), 1, file->f)) {
                    break;
                }
                if (id.timestamp == header.id.timestamp && id.offset == header.id.offset) {
                    documentRef* ref = malloc(sizeof(documentRef));
                    if (ref) {
                        ref->indexNumber = i;
                    }
                    return ref;
                }
            }
        }
    }
    return NULL;
}

void destroyDocumentRef(documentRef* ref) {
    if (ref) {
        free(ref);
    }
}

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
    if (!schema || !key || strlen(key) > 12) {
        return false;
    }
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
    strncpy(el.key, key, 13);
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
    return !value ? false : addElementToSchema(schema, (element) { .type = TYPE_STRING, .stringValue = (str) {
            strlen(value) + 1, value }}, key);
}

bool addEmbeddedDocumentToSchema(documentSchema* schema, char* key, documentSchema* embeddedSchema) {
    if (embeddedSchema && schema != embeddedSchema) {
        return addElementToSchema(schema, (element) { .type = TYPE_EMBEDDED_DOCUMENT, .schemaValue = embeddedSchema },
                                  key);
    }
    return false;
}

bool checkDocument(zgdbFile* file, uint64_t indexNumber, documentSchema* neededSchema, condition* cond) {
    zgdbIndex index = getIndex(file, indexNumber);
    if (index.flag == INDEX_ALIVE) {
        fseeko64(file->f, index.offset, SEEK_SET); // спуск в документ по смещению
        documentHeader header;
        if (fread(&header, sizeof(documentHeader), 1, file->f)) {
            uint64_t bytesRead = sizeof(documentHeader);
            for (uint64_t i = 0; i < neededSchema->elementCount && bytesRead < header.size; i++) {
                element el;
                uint64_t tmp = 0;
                if (!fread(&el.type, sizeof(uint8_t), 1, file->f) || fread(&el.key, sizeof(char), 13, file->f) != 13) {
                    goto exit;
                } else {
                    // Если схема документа отличается от требуемой, то возвращаем false:
                    if (!strcmp(neededSchema->elements[i].key, el.key)) {
                        return false;
                    }
                    bytesRead += sizeof(uint8_t) + sizeof(char) * 13;

                    switch (el.type) {
                        case TYPE_NOT_EXIST:
                            // TODO: выход из документа
                            break;
                        case TYPE_INT:
                            if (!fread(&el.integerValue, sizeof(int32_t), 1, file->f)) {
                                goto exit;
                            }
                            break;
                        case TYPE_DOUBLE:
                            if (!fread(&el.doubleValue, sizeof(double), 1, file->f)) {
                                goto exit;
                            }
                            break;
                        case TYPE_BOOLEAN:
                            if (!fread(&el.booleanValue, sizeof(uint8_t), 1, file->f)) {
                                goto exit;
                            }
                            break;
                        case TYPE_STRING:
                            if (fread(&el.stringValue.size, sizeof(uint32_t), 1, file->f)) {
                                el.stringValue.data = malloc(sizeof(char) * el.stringValue.size);
                                if (el.stringValue.data) {
                                    if (fread(el.stringValue.data, sizeof(char), el.stringValue.size, file->f) !=
                                        el.stringValue.size) {
                                        free(el.stringValue.data);
                                        goto exit;
                                    }
                                }
                            }
                            break;
                        case TYPE_EMBEDDED_DOCUMENT:
                            el.documentValue = malloc(sizeof(documentRef));
                            if (el.documentValue) {
                                if (!fread(&tmp, 5, 1, file->f)) {
                                    free(el.documentValue);
                                    goto exit;
                                }
                                el.documentValue->indexNumber = tmp;
                            }
                            break;
                    }
                    checkCondition(&el, cond);
                }
            }
        }
    }
    exit:
    return NULL;
}

documentRef* findAllDocuments(zgdbFile* file, documentRef* parent, documentSchema* neededSchema, condition* cond) {
    if (parent && parent->indexNumber != DOCUMENT_NOT_EXIST) {
        zgdbIndex index = getIndex(file, parent->indexNumber);
        if (index.flag == INDEX_ALIVE) {
            fseeko64(file->f, index.offset, SEEK_SET); // спуск в родительский документ по смещению
            documentHeader header;
            if (fread(&header, sizeof(documentHeader), 1, file->f)) {
                uint64_t bytesRead = sizeof(documentHeader);
                element el;
                uint64_t childIndexNumber = 0;
                while (bytesRead < header.size) {
                    if (!fread(&el.type, sizeof(uint8_t), 1, file->f) ||
                        fread(&el.key, sizeof(char), 13, file->f) != 13) {
                        goto exit;
                    } else {
                        bytesRead += sizeof(uint8_t) + sizeof(char) * 13;
                        switch (el.type) {
                            case TYPE_NOT_EXIST:
                                bytesRead = header.size;
                                break;
                            case TYPE_INT:
                                fseeko64(file->f, sizeof(int32_t), SEEK_CUR);
                                bytesRead += sizeof(int32_t);
                                break;
                            case TYPE_DOUBLE:
                                fseeko64(file->f, sizeof(double), SEEK_CUR);
                                bytesRead += sizeof(double);
                                break;
                            case TYPE_BOOLEAN:
                                fseeko64(file->f, sizeof(uint8_t), SEEK_CUR);
                                bytesRead += sizeof(uint8_t);
                                break;
                            case TYPE_STRING:
                                if (!fread(&el.stringValue.size, sizeof(uint32_t), 1, file->f)) {
                                    goto exit;
                                }
                                fseeko64(file->f, el.stringValue.size, SEEK_CUR);
                                bytesRead += sizeof(uint32_t) + sizeof(char) * el.stringValue.size;
                                break;
                            case TYPE_EMBEDDED_DOCUMENT:
                                if (!fread(&childIndexNumber, 5, 1, file->f)) {
                                    goto exit;
                                }
                                bytesRead += 5;

                                if (childIndexNumber != DOCUMENT_NOT_EXIST &&
                                    checkDocument(file, childIndexNumber, neededSchema, cond)) {
                                    // TODO: добавить documentRef в вывод функции
                                }
                                break;
                        }
                    }
                }
            }
        }
    }
    exit:
    return NULL;
}