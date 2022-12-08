#include <malloc.h>
#include <string.h>
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
            // TODO: очистка вложенных документов (сейчас внутренности не чистятся!)
            for (int i = 0; i < schema->elementNumber; i++) {
                if ((schema->elements[i].type == TYPE_EMBEDDED_DOCUMENT || schema->elements[i].type == TYPE_STRING) &&
                    schema->elements[i].documentValue) {
                    free(schema->elements[i].documentValue);
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
        // TODO: опять же, если документ вложенный, он нормально не почистится!
        for (int i = 0; i < schema->elementNumber; i++) {
            if ((schema->elements[i].type == TYPE_EMBEDDED_DOCUMENT || schema->elements[i].type == TYPE_STRING) &&
                schema->elements[i].documentValue) {
                free(schema->elements[i].documentValue);
            }
        }
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
                size += calcDocumentSize(elements[i].documentValue->elements, elements[i].documentValue->elementNumber);
                break;
        }
    }
    return size;
}

bool writeElement(zgdbFile* file, element* el) {
    // TODO: проверки
    fwrite(&el->type, sizeof(uint8_t), 1, file->f);
    fwrite(el->key, sizeof(char), 13, file->f);
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
            for (int i = 0; i < el->documentValue->elementNumber; i++) {
                writeElement(file, el->documentValue->elements + i);
            }
            // TODO: дописать рекурсивный вызов записи документа
            break;
    }
    return true;
}

bool writeDocument(zgdbFile* file, sortedList* list, documentSchema* schema) {
    if (list->front) {
        documentHeader* header = malloc(sizeof(documentHeader));
        if (header) {
            header->size = calcDocumentSize(schema->elements, schema->elementNumber);
            uint8_t flag = INDEX_ALIVE;
            // TODO: оптимизировать ветки (вынести что можно наружу?)
            if (list->front->size >= header->size) {
                fseeko64(file->f, getIndex(file, list->front->index)->offset, SEEK_SET);
                header->indexOrder = list->front->index;
                updateIndex(file, list->front->index, &flag, NULL);
                popFront(list);
            } else if (list->back->size >= header->size) {
                fseeko64(file->f, getIndex(file, list->back->index)->offset, SEEK_SET);
                header->indexOrder = list->back->index;
                updateIndex(file, list->back->index, &flag, NULL);
                popBack(list);
            } else if (list->back->size == 0) {
                fseeko64(file->f, 0, SEEK_END);
                uint64_t offset = ftello64(file->f);
                header->indexOrder = list->back->index;
                updateIndex(file, list->back->index, &flag, &offset);
                popBack(list);
            } else {
                // TODO: перемещение первого блока и выделение новых индексов
            }

            // TODO: добавить в header id
            if (fwrite(header, sizeof(documentHeader), 1, file->f)) {
                for (int i = 0; i < schema->elementNumber; i++) {
                    writeElement(file, schema->elements + i);
                }
            }
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

element* readElementFromDocument(zgdbFile* file, const char* neededKey, uint64_t i) {
    element* el = malloc(sizeof(element));
    zgdbIndex* index = getIndex(file, i);
    if (index) {
        fseeko64(file->f, index->offset + sizeof(documentHeader), SEEK_SET);
        bool matchFound;
        do {
            fread(&el->type, sizeof(uint8_t), 1, file->f);
            fread(&el->key, sizeof(char), 13, file->f);
            printf("%s\n", el->key);
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
                    // TODO: дописать рекурсивный вызов чтения документа
                    break;
            }
        } while (!matchFound);
        free(index);
    }
    return el;
}