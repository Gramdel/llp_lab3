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


    documentSchema* schema1 = createSchema(2);
    if (schema1) {
        addIntegerToSchema(schema1, "first1", 111);
        addIntegerToSchema(schema1, "first2", 222);
        addBooleanToSchema(schema1, "isFirst", true);
    }

    documentSchema* schema2 = createSchema(2);
    if (schema2) {
        addIntegerToSchema(schema2, "key1", 123);
        addIntegerToSchema(schema2, "key2", 456);
        addIntegerToSchema(schema2, "key3", 789);
    }

    documentSchema* schema3 = createSchema(2);
    if (schema3) {
        addIntegerToSchema(schema3, "key1", 123);
        addIntegerToSchema(schema3, "key2", 456);
        addIntegerToSchema(schema3, "key3", 789);
        addBooleanToSchema(schema3, "isFirst", false);
        addBooleanToSchema(schema3, "isFoo", true);
    }

    //writeDocument(file, list, schema1);
    writeDocument(file, list, schema2);
    writeDocument(file, list, schema3);
    writeDocument(file, list, schema2);
    writeDocument(file, list, schema2);
    writeDocument(file, list, schema2);
    writeDocument(file, list, schema2);
    writeDocument(file, list, schema2);
    writeDocument(file, list, schema2);
    writeDocument(file, list, schema2);

    writeDocument(file, list, schema2);

    element* el = readElement(file, "first1", 0);
    printf("%s\n", el->key);

    if (list) {
        destroyList(list);
    }
    destroySchema(schema1);
    destroySchema(schema2);
    destroySchema(schema3);

    closeFile(file);
    return 0;
}