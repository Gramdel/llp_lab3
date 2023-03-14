#include <stdio.h>
#include <malloc.h>
#include "zgdb/document.h"

int main(int argc, char** argv) {
    zgdbFile* file = loadFile("test");
    if (!file) {
        file = createFile("test");
        if (!file) {
            printf("Error\n");
            exit(-1);
        }
    }

    documentSchema* root1Schema = createSchema(6); // size
    if (root1Schema) {
        addBooleanToSchema(root1Schema, "isFirst", false);
        addBooleanToSchema(root1Schema, "isFoo", true);
        addStringToSchema(root1Schema, "testString", "BLA");
    }

    documentSchema* grandChildSchema = createSchema(3); // size
    if (grandChildSchema) {
        addIntegerToSchema(grandChildSchema, "grChildInt1", 123);
        addIntegerToSchema(grandChildSchema, "grChildInt2", 456);
        addIntegerToSchema(grandChildSchema, "grChildInt3", 789);
    }

    documentSchema* childSchema = createSchema(3); // size
    if (childSchema) {
        addIntegerToSchema(childSchema, "childInt1", 111);
        addIntegerToSchema(childSchema, "childInt2", 222);
        addEmbeddedDocumentToSchema(childSchema, "grandChild", grandChildSchema);
    }

    documentSchema* root2Schema = createSchema(2); // size
    if (root2Schema) {
        addIntegerToSchema(root2Schema, "rootInt1", 123);
        addIntegerToSchema(root2Schema, "rootInt2", 456);
        addIntegerToSchema(root2Schema, "rootInt3", 789);
        addBooleanToSchema(root2Schema, "isFirst", true);
        addDoubleToSchema(root2Schema, "rootDouble", 128.128);
        addStringToSchema(root2Schema, "rootString", "I AM ROOT");
        addEmbeddedDocumentToSchema(root2Schema, "child", childSchema);
    }
    //documentRef* root1 = getDocumentByID(file, "63AC2DFE000000000000006C");
    documentRef* root1 = writeDocument(file, root1Schema);
    documentRef* root2 = writeDocument(file, root2Schema);

    printDocument(file, root1);
    printDocument(file, root2);

    condition* cond = condOr(condEqual(intElement("childInt11", 111)), condLess(intElement("childInt2", 250)));

    printf("RESULT OF FIND:\n");
    findAllDocuments(file, root2, childSchema, cond);

    destroyDocumentRef(root1);
    destroyDocumentRef(root2);

    destroySchema(childSchema);
    destroySchema(grandChildSchema);
    destroySchema(root1Schema);
    destroySchema(root2Schema);

    closeFile(file);
    return 0;
}