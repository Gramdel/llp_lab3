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

    documentSchema* rootSchema = createSchema("root"); // length
    if (rootSchema) {
        addElementToSchema(rootSchema, intElement("rootInt1", 123));
        addElementToSchema(rootSchema, intElement("rootInt2", 456));
        addElementToSchema(rootSchema, intElement("rootInt3", 789));
        addElementToSchema(rootSchema, booleanElement("isFirst", true));
        addElementToSchema(rootSchema, doubleElement("rootDouble", 128.128));
        addElementToSchema(rootSchema, stringElement("rootString", ""));
    }

    documentSchema* childSchema = createSchema("child");
    if (childSchema) {
        addElementToSchema(childSchema, intElement("grChildInt1", 111));
        addElementToSchema(childSchema, intElement("grChildInt2", 222));
    }

    documentSchema* grandChildSchema = createSchema("grandChild");
    if (grandChildSchema) {
        addElementToSchema(grandChildSchema, intElement("grChildInt1", 123));
        addElementToSchema(grandChildSchema, intElement("grChildInt2", 456));
        addElementToSchema(grandChildSchema, intElement("grChildInt3", 789));
    }

    documentSchema* newRootValues = createSchema("root");
    if (newRootValues) {
        addElementToSchema(newRootValues, stringElement("rootString", "HEY BITCH!"));
    }

    documentSchema* newChildValues = createElements();
    if (childSchema) {
        addElementToSchema(newChildValues, intElement("childInt1", 121));
        addElementToSchema(newChildValues, intElement("childInt1", 212));
    }

    documentSchema* newGrandChildValues = createElements();
    if (grandChildSchema) {
        addElementToSchema(newGrandChildValues, intElement("grChildInt1", 505));
        addElementToSchema(newGrandChildValues, intElement("grChildInt3", 987));
    }

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
        query* updateChild = createUpdateQuery("child", newChildValues, NULL);
        addNestedQuery(updateChild, createUpdateQuery("grandChild", newGrandChildValues, NULL));
        addNestedQuery(update, updateChild);
    }

    query* delete = createDeleteQuery("root", NULL);

    bool error;
    printf(executeInsert(file, &error, insert) ? "true\n" : "false\n");
    iterator* it = NULL;
    executeSelect(file, &error, &it, selectRoot);
    while (hasNext(it)) {
        document* doc = next(file, it);
        printDocumentAsTree(file, doc);
        destroyDocument(doc);
    }
    destroyIterator(it);

    printf(executeInsert(file, &error, insert2) ? "true\n" : "false\n");
    executeSelect(file, &error, &it, selectRoot);
    while (hasNext(it)) {
        document* doc = next(file, it);
        printDocumentAsTree(file, doc);
        destroyDocument(doc);
    }
    destroyIterator(it);


    printf(executeUpdate(file, &error, update) ? "true\n" : "false\n");
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
        printDocumentAsTree(file, doc);
        destroyDocument(doc);
    }
    destroyIterator(it);

    destroySchema(rootSchema);

    closeFile(file);
    return 0;
}