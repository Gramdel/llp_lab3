#include <stdio.h>
#include <malloc.h>
#include "zgdb/document.h"
#include "zgdb/format.h"
#include "zgdb/list.h"

int main(int argc, char** argv) {
    zgdbFile* file = loadFile("test");
    if (!file) {
        file = createFile("test");
        if (!file) {
            printf("Error\n");
            exit(-1);
        }
    }
    printf("%08X\n", file->header.fileType);

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

    documentSchema* schema4 = createSchema(2); // size ?
    if (schema4) {
        addIntegerToSchema(schema4, "fou1", 123);
        addIntegerToSchema(schema4, "fou2", 456);
        addIntegerToSchema(schema4, "fou3", 789);
        addBooleanToSchema(schema4, "isFirst", false);
        addDoubleToSchema(schema4, "testDouble", 128.128);
        addStringToSchema(schema4, "testString", "I AM STRING");
        addDocumentToSchema(schema4, "testDoc", 0);
    }

    writeDocument(file, schema1); // 0
    writeDocument(file, schema2); // 1
    writeDocument(file, schema3); // 2
    writeDocument(file, schema4); // 3
    writeDocument(file, schema2); // 4
    writeDocument(file, schema2); // 5
    writeDocument(file, schema2); // 6
    writeDocument(file, schema2); // 7
    writeDocument(file, schema2); // 8
    writeDocument(file, schema2); // 9

    printf("%d\n", readElement(file, "thi3", 20).integerValue);
    removeDocument(file, 2);
    writeDocument(file, schema2); // 10

    destroySchema(schema1);
    destroySchema(schema2);
    destroySchema(schema3);
    destroySchema(schema4);

    closeFile(file);
    return 0;
}