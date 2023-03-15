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

    documentSchema* grandChildSchema = createSchema("grandChild", 3); // length
    if (grandChildSchema) {
        addIntegerToSchema(grandChildSchema, "grChildInt1", 123);
        addIntegerToSchema(grandChildSchema, "grChildInt2", 456);
        addIntegerToSchema(grandChildSchema, "grChildInt3", 789);
    }

    documentSchema* childSchema = createSchema("child", 3); // length
    if (childSchema) {
        addIntegerToSchema(childSchema, "childInt1", 111);
        addIntegerToSchema(childSchema, "childInt2", 222);
        addEmbeddedDocumentToSchema(childSchema, "grandChild", grandChildSchema);
    }

    documentSchema* root2Schema = createSchema("root2", 2); // length
    if (root2Schema) {
        addIntegerToSchema(root2Schema, "rootInt1", 123);
        addIntegerToSchema(root2Schema, "rootInt2", 456);
        addIntegerToSchema(root2Schema, "rootInt3", 789);
        addBooleanToSchema(root2Schema, "isFirst", true);
        addDoubleToSchema(root2Schema, "rootDouble", 128.128);
        addStringToSchema(root2Schema, "rootString", "I AM ROOT");
        addEmbeddedDocumentToSchema(root2Schema, "child", childSchema);
    }

    createRoot(file, root2Schema);

    condition* cond = condOr(condEqual(intElement("childInt1", 111)), condEqual(intElement("grChildInt2", 456)));
    query* q = createQuery("root2", NULL, 2,
                           createQuery("child", cond, 1,
                                       createQuery("grandChild", cond, 0)),
                           createQuery("child", cond, 0));

    printf("RESULT OF FIND:\n");
    iterator* it = executeQuery(file, q);
    while (hasNext(it)) {
        document* doc = next(file, it);
        printDocument(file, doc);
        destroyDocument(doc);
    }

    //destroyDocumentRef(root2);

    destroySchema(childSchema);
    destroySchema(grandChildSchema);
    destroySchema(root2Schema);

    closeFile(file);
    return 0;
}