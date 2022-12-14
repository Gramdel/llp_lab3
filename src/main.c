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


    documentSchema* schema1 = createSchema(2); // size 78
    if (schema1) {
        addIntegerToSchema(schema1, "first1", 111);
        addIntegerToSchema(schema1, "first2", 222);
        addBooleanToSchema(schema1, "isFirst", true);
    }

    documentSchema* schema2 = createSchema(2); // size 81
    if (schema2) {
        addIntegerToSchema(schema2, "sec1", 123);
        addIntegerToSchema(schema2, "sec2", 456);
        addIntegerToSchema(schema2, "sec3", 789);
    }

    documentSchema* schema3 = createSchema(2); // size 111
    if (schema3) {
        addIntegerToSchema(schema3, "thi1", 123);
        addIntegerToSchema(schema3, "thi2", 456);
        addIntegerToSchema(schema3, "thi3", 789);
        addBooleanToSchema(schema3, "isFirst", false);
        addBooleanToSchema(schema3, "isFoo", true);
    }

    documentSchema* schema4 = createSchema(2); // size 126
    if (schema4) {
        addIntegerToSchema(schema4, "fou1", 123);
        addIntegerToSchema(schema4, "fou2", 456);
        addIntegerToSchema(schema4, "fou3", 789);
        addBooleanToSchema(schema4, "isFirst", false);
        addBooleanToSchema(schema4, "isMoo", true);
        addBooleanToSchema(schema4, "isBoo", false);
    }

    writeDocument(file, list, schema1); // 0
    writeDocument(file, list, schema2); // 1
    writeDocument(file, list, schema3); // 2
    writeDocument(file, list, schema2); // 3
    writeDocument(file, list, schema2); // 4
    writeDocument(file, list, schema2); // 5
    writeDocument(file, list, schema2); // 6
    writeDocument(file, list, schema2); // 7
    writeDocument(file, list, schema2); // 8
    writeDocument(file, list, schema2); // 9

    printf("%d\n", readElement(file, "thi3", 20).integerValue);
    removeDocument(file, list, 2);
    writeDocument(file, list, schema2); // 10

    if (list) {
        destroyList(list);
    }
    destroySchema(schema1);
    destroySchema(schema2);
    destroySchema(schema3);
    destroySchema(schema4);

    closeFile(file);
    return 0;
}