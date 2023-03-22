#include <stdio.h>
#include <malloc.h>
#include "zgdb/document_public.h"
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

    query* insert = createInsertQuery(NULL, rootSchema, NULL);
    if (insert) {
        query* insertChild = createInsertQuery(NULL, childSchema, NULL);
        addNestedQuery(insertChild, createInsertQuery(NULL, grandChildSchema, NULL));
        addNestedQuery(insert, insertChild);
    }

    query* insert2 = createInsertQuery("root", NULL, NULL);
    if (insert2) {
        query* insertChild = createInsertQuery("child", NULL, NULL);
        addNestedQuery(insertChild, createInsertQuery(NULL, grandChildSchema, NULL));
        addNestedQuery(insert2, insertChild);
        addNestedQuery(insert2, createInsertQuery(NULL, childSchema, NULL));
    }

    query* select = createSelectQuery("root", NULL);
    if (insert) {
        query* selectChild = createSelectQuery("child", NULL);
        addNestedQuery(selectChild, createSelectQuery("grandChild", NULL));
        addNestedQuery(select, selectChild);
        addNestedQuery(select, createSelectQuery("child", NULL));
    }

    query* selectRoot = createSelectQuery("root", NULL);

    query* update = createUpdateQuery("root", NULL, NULL);
    if (update) {
        query* updateChild = createUpdateQuery("child", newChildSchema, NULL);
        addNestedQuery(updateChild, createUpdateQuery("grandChild", newGrandChildSchema, NULL));
        addNestedQuery(update, updateChild);
    }

    query* delete = createDeleteQuery("root", NULL);

    bool error;
    printf(executeInsert(file, &error, insert) ? "true\n" : "false\n");
    iterator* it = NULL;
    executeSelect(file, &error, &it, selectRoot);
    while (hasNext(it)) {
        document* doc = next(file, it);
        //printDocument(doc);
        printDocumentAsTree(file, doc);
        destroyDocument(doc);
    }
    destroyIterator(it);

    printf(executeInsert(file, &error, insert2) ? "true\n" : "false\n");
    executeSelect(file, &error, &it, selectRoot);
    while (hasNext(it)) {
        document* doc = next(file, it);
        //printDocument(doc);
        printDocumentAsTree(file, doc);
        destroyDocument(doc);
    }
    destroyIterator(it);


    printf(executeUpdate(file, &error,update) ? "true\n" : "false\n");
    executeSelect(file, &error, &it, select);
    while (hasNext(it)) {
        document* doc = next(file, it);
        printDocument(doc);
        //printDocumentAsTree(file, doc);
        destroyDocument(doc);
    }
    destroyIterator(it);

    printf(executeDelete(file, &error, delete) ? "true\n" : "false\n");
    executeSelect(file, &error, &it, selectRoot);
    while (hasNext(it)) {
        document* doc = next(file, it);
        //printDocument(doc);
        printDocumentAsTree(file, doc);
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