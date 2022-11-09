#include <stdio.h>
#include <malloc.h>
#include "zgdb/document.h"
#include "zgdb/format.h"

int main(int argc, char** argv) {
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
    destroySchema(schema);

    zgdbFile* file = loadFile("test");
    if (!file) {
        file = createFile("test");
        if (!file) {
            printf("Error\n");
        }
    }

    uint8_t flag = INDEX_ALIVE;
    uint64_t offset = 0xBBBBBBBBBBBBBBBB;
    updateIndex(file, 4, &flag, &offset);
    zgdbIndex* index = getIndex(file, 4);
    free(index);
    printf("%08X\n", file->header->fileType);
    closeFile(file);

    return 0;
}