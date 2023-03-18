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

    documentSchema* childSchema = createSchema("child", 3,
                                               intElement("childInt1", 111),
                                               intElement("childInt2", 222),
                                               embeddedDocumentElement("grandChild", grandChildSchema));

    documentSchema* newChildSchema = createSchema("child", 3,
                                               intElement("childInt1", 121),
                                               intElement("childInt2", 212));

    documentSchema* rootSchema = createSchema("root", 11,
                                              intElement("rootInt1", 123),
                                              intElement("rootInt2", 456),
                                              intElement("rootInt3", 789),
                                              booleanElement("isFirst", true),
                                              doubleElement("rootDouble", 128.128),
                                              stringElement("rootString", "I AM ROOT"),
                                              embeddedDocumentElement("child", childSchema),
                                              embeddedDocumentElement("child2", childSchema),
                                              embeddedDocumentElement("child3", childSchema),
                                              embeddedDocumentElement("child4", childSchema),
                                              embeddedDocumentElement("grandChild", grandChildSchema));

    documentSchema* newRootSchema = createSchema("root", 1, stringElement("rootString", "HEY BITCH!"));

    createRoot(file, rootSchema);
    condition* cond = condOr(condLess(intElement("childInt1", 1000)), condLess(intElement("grChildInt2", 10000)));
    query* selectOrDelete = selectOrDeleteQuery(rootSchema, NULL, 2,
                                                selectOrDeleteQuery(childSchema, cond, 1,
                                                       selectOrDeleteQuery(grandChildSchema, cond, 0)),
                                                selectOrDeleteQuery(childSchema, cond, 0));
    query* update = updateQuery(rootSchema, NULL, newRootSchema,2,
                                updateQuery(childSchema, cond, NULL, 1,
                                            updateQuery(grandChildSchema, cond, newGrandChildSchema, 0)),
                                updateQuery(childSchema, cond, newChildSchema, 0));
    printf("RESULT OF FIND:\n");
    iterator* it = executeSelect(file, selectOrDelete);
    while (hasNext(it)) {
        document* doc = next(file, it);
        printDocument(file, doc);
        destroyDocument(doc);
    }


    iterator* it2 = executeUpdate(file, update);
    while (hasNext(it2)) {
        document* doc = next(file, it2);
        printDocument(file, doc);
        destroyDocument(doc);
    }


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