#include <stdio.h>
#include <malloc.h>
#include "zgdb/document.h"
#include "zgdb/query_public.h"

int main(int argc, char** argv) {
    zgdbFile* file = loadFile("test");
    if (!file) {
        file = createFile("test");
        if (!file) {
            printf("Error\n");
            exit(-1);
        }
    }

    documentSchema* grandChildSchema = createSchema("grandChild", 3,
                                                    intElement("grChildInt1", 123),
                                                    intElement("grChildInt2", 456),
                                                    intElement("grChildInt3", 789));

    documentSchema* newGrandChildSchema = createSchema("grandChild", 2,
                                                    intElement("grChildInt1", 505),
                                                    intElement("grChildInt3", 987));

    documentSchema* childSchema = createSchema("child", 2,
                                               intElement("childInt1", 111),
                                               intElement("childInt2", 222));

    documentSchema* newChildSchema = createSchema("child", 2,
                                               intElement("childInt1", 121),
                                               intElement("childInt2", 212));

    documentSchema* rootSchema = createSchema("root", 6,
                                              intElement("rootInt1", 123),
                                              intElement("rootInt2", 456),
                                              intElement("rootInt3", 789),
                                              booleanElement("isFirst", true),
                                              doubleElement("rootDouble", 128.128),
                                              stringElement("rootString", "I AM ROOT"));

    documentSchema* newRootSchema = createSchema("root", 1, stringElement("rootString", "HEY BITCH!"));
    /*

    createRoot(file, rootSchema);
    */
    condition* cond = condOr(condLess(intElement("childInt1", 1000)), condLess(intElement("grChildInt2", 10000)));

    query* insert = insertQuery(NULL, NULL, rootSchema,1,
                                insertQuery(NULL, NULL, childSchema, 1,
                                            insertQuery(NULL, NULL, grandChildSchema, 0)),
                                insertQuery(NULL, NULL, childSchema, 0));

    query* update = updateQuery("root", NULL, newRootSchema,2,
                                updateQuery("child", cond, NULL, 1,
                                            updateQuery("grandChild", cond, newGrandChildSchema, 0)),
                                updateQuery("child", cond, newChildSchema, 0));

    query* selectOrDelete = selectOrDeleteQuery("root", NULL, 2,
                                                selectOrDeleteQuery("child", cond, 1,
                                                                    selectOrDeleteQuery("grandChild", cond, 0)),
                                                selectOrDeleteQuery("child", NULL, 0));

    printf(executeInsert(file, insert) ? "true\n" : "false\n");
    iterator* it = executeSelect(file, selectOrDelete);
    while (hasNext(it)) {
        document* doc = next(file, it);
        printDocument(file, doc);
        destroyDocument(doc);
    }
    destroyIterator(it);

    printf(executeUpdate(file, update) ? "true\n" : "false\n");
    it = executeSelect(file, selectOrDelete);
    while (hasNext(it)) {
        document* doc = next(file, it);
        printDocument(file, doc);
        destroyDocument(doc);
    }
    destroyIterator(it);

    /*
    iterator* it3 = executeDelete(file, selectOrDelete);
    while (hasNext(it3)) {
        document* doc = next(file, it3);
        printDocument(file, doc);
        destroyDocument(doc);
    }
    */

    destroySchema(rootSchema);

    closeFile(file);
    return 0;
}