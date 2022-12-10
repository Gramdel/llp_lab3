#include <stdio.h>
#include <malloc.h>
#include "zgdb/document.h"
#include "zgdb/format.h"
#include "zgdb/list.h"

int main(int argc, char** argv) {
    sortedList* list;
    zgdbFile* file = loadFile("test");
    if (!file) {
        list = initList();
        file = createFile("test", list);
        if (!file) {
            printf("Error\n");
        }
    } else {
        list = createList(file);
    }
    printf("%08X\n", file->header->fileType);


    documentSchema* schema = createSchema(2);
    if (schema) {
        addIntegerToSchema(schema, "key1", 123);
        addIntegerToSchema(schema, "key2", 456);
        addIntegerToSchema(schema, "key3", 789);
        for (int i = 0; i < schema->elementNumber; i++) {
            printf("elements[%d]: key = %s, value = %d;\n", i, schema->elements[i].key,
                   schema->elements[i].integerValue);
        }
    }
    writeDocument(file, list, schema);
    writeDocument(file, list, schema);
    writeDocument(file, list, schema);

    readElement(file, "key3", 0);
    readElement(file, "key2", 1);
    readElement(file, "key1", 2);

    if (list) {
        destroyList(list);
    }
    destroySchema(schema);

    closeFile(file);
    return 0;
}