#include <malloc.h>
#include <string.h>
#include <time.h>
#include "document.h"

documentSchema* createSchema(size_t capacity) {
    documentSchema* schema = malloc(sizeof(documentSchema));
    if (schema) {
        schema->elements = malloc(sizeof(element) * capacity);
        if (schema->elements) {
            schema->elementNumber = 0;
            schema->capacity = capacity;
        }
    }
    return schema;
}

void destroySchema(documentSchema* schema) {
    if (schema) {
        if (schema->elements) {
            for (int i = 0; i < schema->elementNumber; i++) {
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
bool addElementToSchema(documentSchema* schema, element* el) {
    if (schema->elementNumber == schema->capacity) {
        element* newElements = malloc(sizeof(element) * (schema->capacity + 1));
        if (!newElements) {
            free(el);
            return false;
        }
        memcpy(newElements, schema->elements, sizeof(element) * (schema->capacity++));
        free(schema->elements);
        schema->elements = newElements;
    }
    schema->elements[schema->elementNumber++] = *el;
    free(el);
    return true;
}

bool addIntegerToSchema(documentSchema* schema, const char* key, int32_t value) {
    element* el = malloc(sizeof(element));
    if (el) {
        el->type = TYPE_INT;
        el->integerValue = value;
        memset(el->key, 0, 13);
        strcpy(el->key, key);
        return addElementToSchema(schema, el);
    }
    return false;
}

bool addBooleanToSchema(documentSchema* schema, const char* key, uint8_t value) {
    element* el = malloc(sizeof(element));
    if (el) {
        el->type = TYPE_BOOLEAN;
        el->booleanValue = value;
        memset(el->key, 0, 13);
        strcpy(el->key, key);
        return addElementToSchema(schema, el);
    }
    return false;
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
                size += sizeof(unsigned char) * el.stringValue->size;
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
    fseek(file->f, sizeof(zgdbHeader), SEEK_SET);
    for (int i = 0; i < file->header->indexNumber; i++) {
        fseek(file->f, sizeof(zgdbIndex), SEEK_CUR);
    }
    fseek(file->f, file->header->firstDocumentOffset, SEEK_CUR);

    int64_t oldPos = ftello64(file->f); // сохраняем позицию, с которой начинается первый документ
    int64_t neededSpace = sizeof(zgdbIndex) * ZGDB_DEFAULT_INDEX_CAPACITY;
    int64_t availableSpace = file->header->firstDocumentOffset; // перед первым документом могут быть неиспользуемые байты
    do {
        documentHeader header;
        fread(&header, sizeof(documentHeader), 1, file->f);
        oldPos += sizeof(documentHeader);

        /* Если есть подходящая дырка, делаем следующие операции:
         * 1. Забираем node из списка
         * 2. Добавляем node обратно, изменив size на 0
         * 3. Делаем соответствующий индекс в файле INDEX_NEW
         * 4. Сохраняем из node смещение (fseek + ftell)
         * Таким образом, INDEX_DEAD превращается в INDEX_NEW, а его бывший offset будет прикреплен к индексу
         * переносимого документа.
         * Если подходящих дырок нет (или список пустой), то перемещаемся в конец файла, смещение также сохраняем. */
        if (list->front && list->front->size >= header.size) {
            listNode* node = popFront(list);
            node->size = 0;
            insertNode(list, node);

            uint8_t flag = INDEX_NEW;
            uint64_t offset = 0;
            updateIndex(file, node->index, &flag, &offset);

            zgdbIndex* index = getIndex(file, node->index);
            fseeko64(file->f, index->offset, SEEK_SET);
            free(index);
            free(node);
        } else {
            fseeko64(file->f, 0, SEEK_END);
        }
        int64_t newPos = ftello64(file->f);
        updateIndex(file, header.indexOrder, NULL, (uint64_t*) &newPos); // обновляем offset в индексе переносимого документа

        fwrite(&header, sizeof(documentHeader), 1, file->f); // записываем хедер
        newPos += sizeof(documentHeader);
        int64_t bytesLeft = header.size - sizeof(documentHeader);
        do {
            int64_t bufSize;
            if (bytesLeft > DOCUMENT_BUF_SIZE) {
                bufSize = DOCUMENT_BUF_SIZE;
            } else {
                bufSize = bytesLeft;
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
        fseeko64(file->f, oldPos, SEEK_SET); // перед следующей итерацией нужно вернуться к началу
        availableSpace += header.size; // возможно переполнение, если ZGDB_DEFAULT_INDEX_CAPACITY будет слишком большим!
    } while (availableSpace < neededSpace);

    writeIndexes(file, availableSpace / sizeof(zgdbIndex), list);
    file->header->firstDocumentOffset = availableSpace % sizeof(zgdbIndex); // сохраняем остаток места
    writeHeader(file);
    return true;
}

bool writeElement(zgdbFile* file, element* el) {
    // TODO: проверки
    fwrite(&el->type, sizeof(uint8_t), 1, file->f);
    fwrite(el->key, sizeof(char), 13, file->f);
    uint64_t tmp; // для битового поля (documentValue)
    switch (el->type) {
        case TYPE_INT:
            fwrite(&el->integerValue, sizeof(int32_t), 1, file->f);
            break;
        case TYPE_DOUBLE:
            fwrite(&el->doubleValue, sizeof(double), 1, file->f);
            break;
        case TYPE_BOOLEAN:
            fwrite(&el->booleanValue, sizeof(uint8_t), 1, file->f);
            break;
        case TYPE_STRING:
            fwrite(el->stringValue->data, sizeof(unsigned char), el->stringValue->size, file->f);
            break;
        case TYPE_EMBEDDED_DOCUMENT:
            tmp = el->documentValue;
            fwrite(&tmp, 5, 1, file->f); // uint64_t : 40 == 5 байт
            break;
    }
    return true;
}

bool writeDocument(zgdbFile* file, sortedList* list, documentSchema* schema) {
    if (!list->front) {
        moveFirstDocuments(file, list);
    }

    documentHeader* header = malloc(sizeof(documentHeader));
    if (header) {
        bool updateOffsetInIndex = false;
        header->size = calcDocumentSize(schema->elements, schema->elementNumber);
        header->parentIndexOrder = DOCUMENT_HAS_NO_PARENT; // указывает на то, что родителя нет
        if (list->front->size >= header->size) {
            zgdbIndex* index = getIndex(file, list->front->index);
            header->id.offset = index->offset;
            fseeko64(file->f, header->id.offset, SEEK_SET);
            header->indexOrder = list->front->index;
            popFront(list);
            free(index);
        } else {
            if (list->back->size != 0) {
                moveFirstDocuments(file, list);
            }
            fseeko64(file->f, 0, SEEK_END);
            header->id.offset = ftello64(file->f);
            header->indexOrder = list->back->index;
            popBack(list);
            updateOffsetInIndex = true;
        }

        header->id.timestamp = (uint32_t) time(NULL);
        if (fwrite(header, sizeof(documentHeader), 1, file->f)) {
            for (int i = 0; i < schema->elementNumber; i++) {
                writeElement(file, schema->elements + i);
            }
        }

        uint8_t flag = INDEX_ALIVE;
        uint64_t offset = header->id.offset;
        updateIndex(file, header->indexOrder, &flag, updateOffsetInIndex ? &offset : NULL);
        free(header);
    }
    return true;
}

element* readElement(zgdbFile* file, const char* neededKey, uint64_t i) {
    element* el = malloc(sizeof(element));
    zgdbIndex* index = getIndex(file, i);
    if (index) {
        fseeko64(file->f, index->offset, SEEK_SET); // спуск в документ по смещению
        fseek(file->f, sizeof(documentHeader), SEEK_CUR); // TODO: может нужен fread?
        bool matchFound;
        do {
            fread(&el->type, sizeof(uint8_t), 1, file->f);
            fread(&el->key, sizeof(char), 13, file->f);
            uint64_t tmp; // для битового поля (documentValue)
            matchFound = strcmp(el->key, neededKey) == 0;
            switch (el->type) {
                case TYPE_INT:
                    fread(&el->integerValue, sizeof(int32_t), 1, file->f);
                    break;
                case TYPE_DOUBLE:
                    fread(&el->doubleValue, sizeof(double), 1, file->f);
                    break;
                case TYPE_BOOLEAN:
                    fread(&el->booleanValue, sizeof(uint8_t), 1, file->f);
                    break;
                case TYPE_STRING:
                    if (matchFound) {
                        el->stringValue = malloc(sizeof(str));
                        fread(&el->stringValue->size, sizeof(uint32_t), 1, file->f);
                        el->stringValue->data = malloc(sizeof(unsigned char) * (el->stringValue->size + 1));
                        fread(&el->stringValue->data, sizeof(unsigned char), el->stringValue->size + 1, file->f);
                    } else {
                        fread(&el->integerValue, sizeof(uint32_t), 1, file->f);
                        fseeko64(file->f, el->integerValue, SEEK_CUR);
                    }
                    break;
                case TYPE_EMBEDDED_DOCUMENT:
                    fread(&tmp, 5, 1, file->f); // uint64_t : 40 == 5 байт
                    el->documentValue = tmp;
                    break;
            }
        } while (!matchFound);
        free(index);
    }
    return el;
}