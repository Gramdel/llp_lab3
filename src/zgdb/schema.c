#include <malloc.h>
#include <string.h>

#include "document.h"
#include "schema.h"
#include "element.h"

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