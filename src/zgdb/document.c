#include <malloc.h>
#include <string.h>
#include <time.h>

#include "format.h"
#include "document.h"

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
                /* Спускаемся к хедеру ребёнка и пропускаем метку, размер, номер индекса.
                 * Считываем parentIndexNumber, если он не равен DOCUMENT_NOT_EXIST, то завершаем выполнение: */
                fseeko64(file->f, index.offset + 11, SEEK_SET);
                if (!fread(&tmp, 5, 1, file->f) || tmp != DOCUMENT_NOT_EXIST) {
                    return 0;
                }

                // Снова спускаемся к хедеру и пропускаем метку, размер, номер индекса. Записываем parentIndexNumber:
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

uint64_t writeDocument(zgdbFile* file, documentSchema* schema) {
    documentHeader header;
    header.size = calcDocumentSize(schema->elements, schema->elementCount);
    header.mark = DOCUMENT_START_MARK;
    header.parentIndexNumber = DOCUMENT_NOT_EXIST; // указывает на то, что родителя нет

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
            return DOCUMENT_NOT_EXIST;
        }
    }
    return bytesWritten == header.size ? header.indexNumber : DOCUMENT_NOT_EXIST;
}

bool removeEmbeddedDocument(zgdbFile* file, uint64_t childIndexNumber, uint64_t parentIndexNumber) {
    zgdbIndex index = getIndex(file, childIndexNumber);
    if (index.flag == INDEX_ALIVE) {
        // Считываем хедер документа:
        fseeko64(file->f, index.offset, SEEK_SET);
        documentHeader header;
        if (!fread(&header, sizeof(documentHeader), 1, file->f)) {
            return false;
        }

        /* Проверка на то, совпадает ли родитель удаляемого документа с parentIndexNumber.
         * При попытке удалить вложенный документ напрямую (а не путем удаления его родителя), должен вернуться false: */
        if (parentIndexNumber != header.parentIndexNumber) {
            return parentIndexNumber != DOCUMENT_NOT_EXIST;
        }

        // Записываем вместо хедера документа "хедер дырки", изменяем флаг индекса документа и добавляем дырку в список:
        fseeko64(file->f, index.offset, SEEK_SET);
        if (!writeGapSize(file, (int64_t) header.size) ||
            !updateIndex(file, childIndexNumber, wrap_uint8_t(INDEX_DEAD), not_present_int64_t())) {
            return false;
        }
        insertNode(&file->list, createNode(header.size, childIndexNumber));

        // Удаляем детей документа:
        for (uint64_t i = 0; i < file->header.indexCount; i++) {
            if (i != childIndexNumber) {
                if (!removeEmbeddedDocument(file, i, childIndexNumber)) {
                    return false;
                }
            }
        }
    } else if (index.flag == INDEX_NOT_EXIST) {
        return false;
    }
    return true;
}

bool removeDocument(zgdbFile* file, uint64_t i) {
    return removeEmbeddedDocument(file, i, DOCUMENT_NOT_EXIST);
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

// TODO: подчистить?
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
    uint64_t tmp; // для битового поля (documentValue)
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
            printf("key: \"%s\", documentValue: %l\n", el.key, el.documentValue);
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
    // Сразу выделяем индексы, если список пустой:
    if (!file->list.front && !moveFirstDocuments(file)) {
        return false;
    }

    // Спускаемся к нужному элементу по ключу. Если это не строка, завершаем выполнение:
    if (navigateToElement(file, neededKey, i) != TYPE_STRING) {
        return false;
    }
    int64_t posOfValue = ftello64(file->f);

    // Считываем предыдущий размер строки:
    uint32_t newSize = strlen(value) + 1;
    uint32_t oldSize;
    if (!fread(&oldSize, sizeof(uint32_t), 1, file->f)) {
        return false;
    }

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

    // Решаем, что делать, в зависимости от изменения размера строки:
    int64_t diff = (int64_t) newSize - (int64_t) oldSize;
    if (diff > 0) {
        // Если документ НЕ последний, то ищем ему новое место.
        fseeko64(file->f, 0, SEEK_END);
        if (ftello64(file->f) != index.offset + header.size) {
            int64_t diffWithGap = (int64_t) file->list.front->size - (int64_t) header.size;
            if (diffWithGap >= 0) {
                zgdbIndex gapIndex = getIndex(file, file->list.front->indexNumber);
                updateIndex(file, header.indexNumber, not_present_uint8_t(), wrap_int64_t(gapIndex.offset));
                popFront(&file->list);

                // TODO: дальше код собьёт все оффсеты, полученные ранее. Нужно бы это как-то пофиксить.
                if (diffWithGap) {
                    if (!file->list.back || file->list.back->size) {
                        moveFirstDocuments(file); // если нет хвоста или хвост - INDEX_DEAD, то выделяем новые индексы
                    }
                    listNode* node = popBack(&file->list);
                    updateIndex(file, node->indexNumber, wrap_uint8_t(INDEX_DEAD),
                                wrap_int64_t(index.offset + (int64_t) header.size));
                    node->size = diffWithGap;
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
        }


    } else {
        // Возвращаемся к началу value, перезаписываем размер строки и саму строку:
        fseeko64(file->f, posOfValue, SEEK_SET);
        if (!fwrite(&newSize, sizeof(uint32_t), 1, file->f) ||
            fwrite(value, sizeof(char), newSize, file->f) != newSize) {
            return false;
        }

        // Если появилось свободное место, перемещаем остаток документа так, чтобы дырка оказалась в конце:
        if (diff < 0) {
            // Перемещаем остаток, если он не в конце документа:
            int64_t newPos = ftello64(file->f);
            int64_t oldPos = newPos - diff;
            if (oldPos != index.offset + header.size &&
                !moveData(file, &oldPos, &newPos, index.offset + header.size - oldPos)) {
                return false;
            }

            // После перемещения остатка newPos будет указывать на начало новой дырки. Запишем её размер:
            if (!writeGapSize(file, -diff)) {
                return false;
            }

            // Перезаписываем размер документа:
            header.size += diff;
            fseeko64(file->f, index.offset, SEEK_SET);
            if (!fwrite(&header, sizeof(documentHeader), 1, file->f)) {
                return false;
            }

            // Добавляем дырку в список:
            listNode* node = popBack(&file->list);
            if (!updateIndex(file, node->indexNumber, wrap_uint8_t(INDEX_DEAD), wrap_int64_t(newPos))) {
                return false;
            }
            node->size = -diff;
            insertNode(&file->list, node);
        }
    }
    return true;
}

bool updateDocumentValue(zgdbFile* file, char* neededKey, uint64_t value, uint64_t i) {
    // Спускаемся к нужному элементу по ключу:
    if (navigateToElement(file, neededKey, i) == TYPE_EMBEDDED_DOCUMENT) {
        // Считываем предыдущее значение поля:
        int64_t pos = ftello64(file->f);
        uint64_t oldChildIndexNumber;
        if (fread(&oldChildIndexNumber, 5, 1, file->f)) {
            // Получаем индекс нового ребёнка, если он "жив", продолжаем:
            zgdbIndex index = getIndex(file, value);
            if (index.flag == INDEX_ALIVE) {
                /* Спускаемся к хедеру нового ребёнка и пропускаем метку, размер, номер индекса.
                 * Считываем parentIndexNumber, если он равен DOCUMENT_NOT_EXIST, то продолжаем: */
                fseeko64(file->f, index.offset + 11, SEEK_SET);
                uint64_t parentIndexNumber;
                if (fread(&parentIndexNumber, 5, 1, file->f) && parentIndexNumber == DOCUMENT_NOT_EXIST) {
                    /* Записываем в нового ребёнка номер индекса родителя (i), затем удаляем бывшего ребёнка: */
                    fseeko64(file->f, index.offset + 11, SEEK_SET);
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