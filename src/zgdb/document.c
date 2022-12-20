#include <malloc.h>
#include <string.h>
#include <time.h>

#include "format.h"
#include "document.h"

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
            // Документ не может быть сам себе родителем и ребёнком, поэтому:
            if (el->documentValue == parentIndexNumber) {
                return 0;
            }

            uint64_t tmp = el->documentValue; // для битового поля
            bytesWritten += fwrite(&tmp, 5, 1, file->f) * 5; // uint64_t : 40 == 5 байт

            // Записываем в хедер вложенного документа информацию об этом (создаваемом) документе:
            int64_t pos = ftello64(file->f);
            zgdbIndex index = getIndex(file, el->documentValue);
            if (index.flag != INDEX_ALIVE) {
                return 0;
            } else {
                /* Спускаемся к хедеру ребёнка и пропускаем размер, номер индекса.
                 * Считываем parentIndexNumber, если он не равен DOCUMENT_NOT_EXIST, то завершаем выполнение: */
                fseeko64(file->f, index.offset + 5 + 5, SEEK_SET);
                if (!fread(&tmp, 5, 1, file->f) || tmp != DOCUMENT_NOT_EXIST) {
                    return 0;
                }
                // Снова спускаемся к хедеру. Записываем parentIndexNumber:
                fseeko64(file->f, index.offset + 5 + 5, SEEK_SET);
                if (!fwrite(&parentIndexNumber, 5, 1, file->f)) {
                    return 0;
                }
            }
            fseeko64(file->f, pos, SEEK_SET);
            break;
    }
    return bytesWritten;
}

uint64_t writeDocument(zgdbFile* file, documentSchema* schema) {
    documentHeader header;
    header.size = calcDocumentSize(schema->elements, schema->elementCount);
    header.parentIndexNumber = DOCUMENT_NOT_EXIST; // указывает на то, что родителя нет

    // Сразу выделяем индексы, если список пустой:
    if (!file->list.front && !moveFirstDocuments(file)) {
        return DOCUMENT_NOT_EXIST;
    }
    // Если есть подходящая дырка, то пишем документ туда:
    uint64_t newSize = 0;
    int64_t diff = (int64_t) file->list.front->size - (int64_t) header.size;
    if (diff >= 0) {
        // Считываем индекс дырки и обновляем его (делаем INDEX_ALIVE):
        zgdbIndex index = getIndex(file, file->list.front->indexNumber);
        if (index.flag != INDEX_DEAD ||
            !updateIndex(file, file->list.front->indexNumber, wrap_uint8_t(INDEX_ALIVE), not_present_int64_t())) {
            return DOCUMENT_NOT_EXIST;
        }
        // Заполняем заголовок документа:
        header.indexNumber = file->list.front->indexNumber;
        header.id.offset = index.offset;
        if (diff) {
            newSize = file->list.front->size;
        }
        free(popFront(&file->list));
    } else {
        // В любом случае будем писать в конец файла, но, возможно, надо выделить новые индексы. Затем обновляем индекс дырки:
        if (file->list.back->size != 0 && !moveFirstDocuments(file) ||
            !updateIndex(file, file->list.back->indexNumber, wrap_uint8_t(INDEX_ALIVE),
                         wrap_int64_t(file->header.fileSize))) {
            return DOCUMENT_NOT_EXIST;
        }
        // Заполняем заголовок документа:
        header.indexNumber = file->list.back->indexNumber;
        header.id.offset = file->header.fileSize;
        free(popBack(&file->list));
        // Обновляем размер файла
        file->header.fileSize += (int64_t) header.size;
        if (!writeHeader(file)) {
            return DOCUMENT_NOT_EXIST;
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
    return bytesLeft ? DOCUMENT_NOT_EXIST : header.indexNumber;
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

        /* Проверка на то, совпадает ли родитель удаляемого документа с parentIndexNumber.
         * При попытке удалить вложенный документ напрямую (а не путем удаления его родителя), должен вернуться INDEX_NOT_EXIST.
         * Если же идёт поиск детей (parentIndexNumber != DOCUMENT_NOT_EXIST), то нужно вернуть indexFlag: */
        if (parentIndexNumber != header.parentIndexNumber) {
            return parentIndexNumber == DOCUMENT_NOT_EXIST ? INDEX_NOT_EXIST : index.flag;
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

bool removeDocument(zgdbFile* file, uint64_t i) {
    return removeEmbeddedDocument(file, i, DOCUMENT_NOT_EXIST) != INDEX_NOT_EXIST;
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

elementType navigateToElement(zgdbFile* file, char* neededKey, uint64_t i) {
    zgdbIndex index = getIndex(file, i);
    if (index.flag == INDEX_ALIVE) {
        fseeko64(file->f, index.offset, SEEK_SET); // спуск в документ по смещению
        documentHeader header;
        if (fread(&header, sizeof(documentHeader), 1, file->f)) {
            uint64_t bytesRead = sizeof(documentHeader);
            element el;
            while (bytesRead < header.size) {
                if (!fread(&el.type, sizeof(uint8_t), 1, file->f) || fread(&el.key, sizeof(char), 13, file->f) != 13) {
                    goto exit;
                } else if (strcmp(el.key, neededKey) == 0) {
                    fseeko64(file->f, 0, SEEK_CUR); // без этого вызова любой fwrite будет пытаться записать в eof!
                    return el.type;
                } else {
                    bytesRead += sizeof(uint8_t) + sizeof(char) * 13;
                    switch (el.type) {
                        case TYPE_INT:
                            if (fseeko64(file->f, sizeof(int32_t), SEEK_CUR) != 0) {
                                goto exit;
                            }
                            bytesRead += sizeof(int32_t);
                            break;
                        case TYPE_DOUBLE:
                            if (fseeko64(file->f, sizeof(double), SEEK_CUR) != 0) {
                                goto exit;
                            }
                            bytesRead += sizeof(double);
                            break;
                        case TYPE_BOOLEAN:
                            if (fseeko64(file->f, sizeof(uint8_t), SEEK_CUR) != 0) {
                                goto exit;
                            }
                            bytesRead += sizeof(uint8_t);
                            break;
                        case TYPE_STRING:
                            if (!fread(&el.stringValue.size, sizeof(uint32_t), 1, file->f) ||
                                fseeko64(file->f, el.stringValue.size, SEEK_CUR) != 0) {
                                goto exit;
                            }
                            bytesRead += sizeof(uint32_t) + sizeof(char) * el.stringValue.size;
                            break;
                        case TYPE_EMBEDDED_DOCUMENT:
                            if (fseeko64(file->f, 5, SEEK_CUR) != 0) {
                                goto exit;
                            }
                            bytesRead += 5;
                            break;
                    }
                }
            }
        }
    }
    exit:
    return TYPE_NOT_EXIST;
}

element readElement(zgdbFile* file, char* neededKey, uint64_t i) {
    uint64_t tmp = 0; // для битового поля (documentValue)
    element el;
    strcpy(el.key, neededKey);
    el.type = navigateToElement(file, neededKey, i);
    switch (el.type) {
        case TYPE_INT:
            if (!fread(&el.integerValue, sizeof(int32_t), 1, file->f)) {
                el.type = TYPE_NOT_EXIST;
            }
            break;
        case TYPE_DOUBLE:
            if (!fread(&el.doubleValue, sizeof(double), 1, file->f)) {
                el.type = TYPE_NOT_EXIST;
            }
            break;
        case TYPE_BOOLEAN:
            if (!fread(&el.booleanValue, sizeof(uint8_t), 1, file->f)) {
                el.type = TYPE_NOT_EXIST;
            }
            break;
        case TYPE_STRING:
            if (fread(&el.stringValue.size, sizeof(uint32_t), 1, file->f)) {
                el.stringValue.data = malloc(sizeof(char) * el.stringValue.size);
                if (el.stringValue.data) {
                    if (fread(el.stringValue.data, sizeof(char), el.stringValue.size, file->f) == el.stringValue.size) {
                        break;
                    }
                    free(el.stringValue.data);
                }
            }
            el.type = TYPE_NOT_EXIST;
            break;
        case TYPE_EMBEDDED_DOCUMENT:
            if (!fread(&tmp, 5, 1, file->f)) {
                el.type = TYPE_NOT_EXIST;
            }
            el.documentValue = tmp;
            break;
    }
    return el;
}

void destroyElement(element el) {
    if (el.type == TYPE_STRING) {
        free(el.stringValue.data);
    }
}

void printElement(element el) {
    switch (el.type) {
        case TYPE_NOT_EXIST:
            printf("Element does not exist!\n");
            break;
        case TYPE_INT:
            printf("key: \"%s\", integerValue: %d\n", el.key, el.integerValue);
            break;
        case TYPE_DOUBLE:
            printf("key: \"%s\", doubleValue: %f\n", el.key, el.doubleValue);
            break;
        case TYPE_BOOLEAN:
            printf("key: \"%s\", booleanValue: %s\n", el.key, el.booleanValue ? "true" : "false");
            break;
        case TYPE_STRING:
            printf("key: \"%s\", stringValue: \"%s\"\n", el.key, el.stringValue.data);
            break;
        case TYPE_EMBEDDED_DOCUMENT:
            printf("key: \"%s\", documentValue: %ld\n", el.key, el.documentValue);
            break;
    }
}

elementType getTypeOfElement(element el) {
    return el.type;
}

int32_t getIntegerValue(element el) {
    return el.integerValue;
}

double getDoubleValue(element el) {
    return el.doubleValue;
}

uint8_t getBooleanValue(element el) {
    return el.booleanValue;
}

// TODO: может тупо возвращать char* ?
str getStringValue(element el) {
    return el.stringValue;
}

uint64_t getDocumentValue(element el) {
    return el.documentValue;
}


bool updateIntegerValue(zgdbFile* file, char* neededKey, int32_t value, uint64_t i) {
    return navigateToElement(file, neededKey, i) == TYPE_INT && fwrite(&value, sizeof(int32_t), 1, file->f);
}

bool updateDoubleValue(zgdbFile* file, char* neededKey, double value, uint64_t i) {
    return navigateToElement(file, neededKey, i) == TYPE_DOUBLE && fwrite(&value, sizeof(double), 1, file->f);
}

bool updateBooleanValue(zgdbFile* file, char* neededKey, uint8_t value, uint64_t i) {
    return navigateToElement(file, neededKey, i) == TYPE_BOOLEAN && fwrite(&value, sizeof(uint8_t), 1, file->f);
}

bool updateStringValue(zgdbFile* file, char* neededKey, char* value, uint64_t i) {
    // Получаем индекс документа:
    zgdbIndex index = getIndex(file, i);
    if (index.flag != INDEX_ALIVE) {
        return false;
    }
    // Перемещаемся к началу обновляемого документа и считываем его заголовок:
    fseeko64(file->f, index.offset, SEEK_SET);
    documentHeader header;
    if (!fread(&header, sizeof(documentHeader), 1, file->f)) {
        return false;
    }
    // Спускаемся к нужному элементу по ключу, сохраняем относительное смещение:
    if (navigateToElement(file, neededKey, i) != TYPE_STRING) {
        return false;
    }
    int64_t offsetOfValue = ftello64(file->f) - index.offset;
    // Считываем предыдущий размер строки:
    uint32_t newSize = strlen(value) + 1;
    uint32_t oldSize;
    if (!fread(&oldSize, sizeof(uint32_t), 1, file->f)) {
        return false;
    }

    // Если строка стала больше, то ей нужно найти новое место:
    int64_t delta = (int64_t) newSize - (int64_t) oldSize; // изменение размера строки
    uint64_t newHeaderSize = 0;
    if (delta > 0) {
        // Если документ не в конце файла, ищем дырку и переносим его:
        if (index.offset + header.size != file->header.fileSize) {
            int64_t oldPos = index.offset;
            int64_t newPos;
            documentHeader gapHeader = header;
            int64_t diff = file->list.front ? (int64_t) file->list.front->size - (int64_t) header.size - delta : -1;
            if (diff >= 0) {
                // Считываем индекс дырки и обновляем в нём смещение:
                zgdbIndex gapIndex = getIndex(file, file->list.front->indexNumber);
                if (gapIndex.flag != INDEX_DEAD ||
                    !updateIndex(file, file->list.front->indexNumber, not_present_uint8_t(),
                                 wrap_int64_t(index.offset))) {
                    return false;
                }
                gapHeader.indexNumber = file->list.front->indexNumber; // записываем в хедер для будущей дырки номер индекса текущей
                newPos = index.offset = gapIndex.offset;
                newHeaderSize = file->list.front->size;
                listNode* node = popFront(&file->list);
                node->size = 0;
                insertNode(&file->list, node);
            } else {
                // На предыдущем месте образуется дырка, следовательно, нужны индексы:
                if ((!file->list.back || file->list.back->size) && !moveFirstDocuments(file)) {
                    return false;
                }
                // Заново считываем индекс документа, поскольку он мог быть перемещён:
                index = getIndex(file, i);
                if (index.flag != INDEX_ALIVE) {
                    return false;
                }
                // Считываем индекс дырки и обновляем в нём смещение и флаг:
                zgdbIndex gapIndex = getIndex(file, file->list.back->indexNumber);
                if (gapIndex.flag != INDEX_NEW ||
                    !updateIndex(file, file->list.back->indexNumber, wrap_uint8_t(INDEX_DEAD),
                                 wrap_int64_t(index.offset))) {
                    return false;
                }
                gapHeader.indexNumber = file->list.back->indexNumber; // записываем в хедер для будущей дырки номер индекса текущей
                listNode* node = popBack(&file->list);
                node->size = header.size;
                insertNode(&file->list, node);
                // Проверяем, переместился документ или нет:
                if (index.offset + header.size != file->header.fileSize) {
                    oldPos = index.offset;
                    newPos = index.offset = file->header.fileSize;
                    file->header.fileSize += (int64_t) header.size + delta;
                } else {
                    newPos = oldPos; // условие для того, чтобы не перемещать документ
                    file->header.fileSize += delta;
                }
                if (!writeHeader(file)) {
                    return false;
                }
            }
            // Перемещаем документ, если он не оказался в конце файла, обновляем смещение в его индексе и записываем на его месте хедер дырки:
            if (newPos != oldPos) {
                if (!updateIndex(file, header.indexNumber, not_present_uint8_t(), wrap_int64_t(newPos)) ||
                    !moveData(file, &oldPos, &newPos, header.size)) {
                    return false;
                }
                fseeko64(file->f, oldPos - (int64_t) header.size, SEEK_SET);
                if (!fwrite(&gapHeader, sizeof(documentHeader), 1, file->f)) {
                    return false;
                }
            }
        } else {
            file->header.fileSize += delta;
            if (!writeHeader(file)) {
                return false;
            }
        }
    }

    // Перемещаем кусок документа после строки, чтобы не перекрыть его новой строкой или чтобы не было дырок:
    int64_t oldPos = index.offset + offsetOfValue + (int64_t) sizeof(uint32_t) + oldSize;
    int64_t newPos = oldPos + delta;
    if (!moveData(file, &oldPos, &newPos, index.offset + header.size - oldPos)) {
        return false;
    }
    // Возвращаемся к началу value, перезаписываем размер строки и саму строку:
    fseeko64(file->f, index.offset + offsetOfValue, SEEK_SET);
    if (!fwrite(&newSize, sizeof(uint32_t), 1, file->f) || fwrite(value, sizeof(char), newSize, file->f) != newSize) {
        return false;
    }
    // Перезаписываем размер документа:
    header.size = newHeaderSize ? newHeaderSize : header.size + delta;
    fseeko64(file->f, index.offset, SEEK_SET);
    if (!fwrite(&header, sizeof(documentHeader), 1, file->f)) {
        return false;
    }
    return true;
}

bool updateDocumentValue(zgdbFile* file, char* neededKey, uint64_t value, uint64_t i) {
    // Спускаемся к нужному элементу по ключу:
    if (navigateToElement(file, neededKey, i) == TYPE_EMBEDDED_DOCUMENT) {
        // Считываем предыдущее значение поля:
        int64_t pos = ftello64(file->f);
        uint64_t oldChildIndexNumber = 0;
        if (fread(&oldChildIndexNumber, 5, 1, file->f) && oldChildIndexNumber != value) {
            // Получаем индекс нового ребёнка, если он "жив", продолжаем:
            zgdbIndex index = getIndex(file, value);
            if (index.flag == INDEX_ALIVE) {
                /* Спускаемся к хедеру нового ребёнка и пропускаем размер, номер индекса.
                 * Считываем parentIndexNumber, если он равен DOCUMENT_NOT_EXIST, то продолжаем: */
                fseeko64(file->f, index.offset + 5 + 5, SEEK_SET);
                uint64_t parentIndexNumber = 0;
                if (fread(&parentIndexNumber, 5, 1, file->f) && parentIndexNumber == DOCUMENT_NOT_EXIST) {
                    /* Записываем в нового ребёнка номер индекса родителя (i), затем удаляем бывшего ребёнка: */
                    fseeko64(file->f, index.offset + 5 + 5, SEEK_SET);
                    if (fwrite(&i, 5, 1, file->f) && removeEmbeddedDocument(file, oldChildIndexNumber, i)) {
                        // Возвращаемся к обновляемому полю и записываем новое значение:
                        fseeko64(file->f, pos, SEEK_SET);
                        return fwrite(&value, 5, 1, file->f);
                    }
                }
            }
        }
    }
    return false;
}