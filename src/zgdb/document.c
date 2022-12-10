#define NO_PARENT 0xFFFFFFFFFF // максимальный номер индекса - (2^40-1), поэтому 2^40 можно использовать как флаг

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
    if (list->front) {
        documentHeader* header = malloc(sizeof(documentHeader));
        if (header) {
            bool updateOffsetInIndex = false;
            header->size = calcDocumentSize(schema->elements, schema->elementNumber);
            header->parentIndexOrder = NO_PARENT; // указывает на то, что родителя нет
            if (list->front->size >= header->size) {
                header->id.offset = getIndex(file, list->front->index)->offset;
                fseeko64(file->f, header->id.offset, SEEK_SET);
                header->indexOrder = list->front->index;
                popFront(list);
            } else if (list->back->size == 0) {
                fseeko64(file->f, 0, SEEK_END);
                header->id.offset = ftello64(file->f);
                header->indexOrder = list->back->index;
                popBack(list);
                updateOffsetInIndex = true;
            } else {
                // TODO: перемещение первого блока и выделение новых индексов. Не забыть о том, что при перемещении
                //  на место бывшего блока, нужно подменить его (поменять местами offset)
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
    } else {
        // TODO: перемещение первого блока и выделение новых индексов
    }
    return true;
}

bool moveFirstDocument(zgdbFile* file, sortedList* list) {

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