#include <malloc.h>
#include <string.h>

#include "schema.h"
#include "element.h"

documentSchema* createSchema(const char* name, uint64_t capacity) {
    if (name && strlen(name) <= 12) {
        documentSchema* schema = malloc(sizeof(documentSchema));
        if (schema) {
            schema->elementCount = 0;
            schema->capacity = capacity;
            schema->elements = malloc(sizeof(element) * capacity);
            strcpy(schema->name, name);
            // Если capacity == 0 или получилось выделить место, то всё хорошо:
            if (!capacity || schema->elements) {
                return schema;
            }
            free(schema);
        }
    }
    return NULL;
}

void destroySchema(documentSchema* schema) {
    if (schema) {
        if (schema->elements) {
            free(schema->elements);
        }
        free(schema);
    }
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

bool addElementToSchema(documentSchema* schema, element el, const char* key) {
    if (!schema || !key || strlen(key) > 12) {
        return false;
    }
    if (schema->elementCount == schema->capacity) {
        element* tmp = realloc(schema->elements, sizeof(element) * (++schema->capacity));
        if (!tmp) {
            destroySchema(schema);
            return false;
        }
        schema->elements = tmp;
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