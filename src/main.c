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

    documentSchema* childSchema = createSchema("child", 3,
                                               intElement("childInt1", 111),
                                               intElement("childInt2", 222),
                                               embeddedDocumentElement("grandChild", grandChildSchema));

    documentSchema* rootSchema = createSchema("root", 7,
                                              intElement("rootInt1", 123),
                                              intElement("rootInt2", 456),
                                              intElement("rootInt3", 789),
                                              booleanElement("isFirst", true),
                                              doubleElement("rootDouble", 128.128),
                                              stringElement("rootString", "I AM ROOT"),
                                              embeddedDocumentElement("child", childSchema));

    createRoot(file, rootSchema);

    condition* cond = condOr(condEqual(intElement("childInt1", 111)), condEqual(intElement("grChildInt2", 456)));
    query* selectOrDelete = selectOrDeleteQuery("root", NULL, 2,
                                                selectOrDeleteQuery("child", cond, 1,
                                                       selectOrDeleteQuery("grandChild", cond, 0)),
                                                selectOrDeleteQuery("child", cond, 0));
    query* update = updateQuery("root", NULL, rootSchema,2,
                                updateQuery("child", cond, NULL, 1,
                                            updateQuery("grandChild", cond, rootSchema, 0)),
                                updateQuery("child", cond, rootSchema, 0));
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

    destroySchema(childSchema);
    destroySchema(grandChildSchema);
    destroySchema(rootSchema); // TODO: разобраться с очисткой строк и вложенных схем

    closeFile(file);
    return 0;
}