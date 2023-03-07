#include <stdio.h>
#include <malloc.h>
#include "zgdb/document_public.h"
#include "zgdb/element_public.h"
#include "zgdb/query.h"

int main(int argc, char** argv) {
    zgdbFile* file = loadFile("test");
    if (!file) {
        file = createFile("test");
        if (!file) {
            printf("Error\n");
            exit(-1);
        }
    }

    /*

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

    updateDocumentValue(file, "child", grandChildSchema, root2);
    printDocument(file, root2);

    /*
    documentRef* root3 = writeDocument(file, grandChildSchema);
    writeDocument(file, root1Schema);
    writeDocument(file, root1Schema);
    writeDocument(file, root1Schema);
    writeDocument(file, root1Schema);
    writeDocument(file, root1Schema);

    removeDocument(file, root3);
    printDocument(file, root3);

    printDocument(file, writeDocument(file, childSchema));
    printDocument(file, root1);

    */


    /*
    printf("updated? %d\n", updateIntegerValue(file, "rootInt1", 808, root2));
    printf("updated? %d\n", updateBooleanValue(file, "isFirst", false, root2));
    printf("updated? %d\n", updateDoubleValue(file, "rootDouble", -2.5, root2));
    printf("updated? %d\n", updateStringValue(file, "rootString", "I AM STRINGG", root2));
    //printf("updated? %d\n", updateDocumentValue(file, "child", grandChildSchema, root2));

    element* el1 = readElement(file, "rootInt1", root2);
    element* el2 = readElement(file, "isFirst", root2);
    element* el3 = readElement(file, "rootDouble", root2);
    element* el4 = readElement(file, "rootString", root2);
    element* el5 = readElement(file, "child", root2);

    printElement(file, el1);
    printElement(file, el2);
    printElement(file, el3);
    printElement(file, el4);
    printElement(file, el5);

    printDocument(file, root2);

    destroyElement(el1);
    destroyElement(el2);
    destroyElement(el3);
    destroyElement(el4);
    destroyElement(el5);
    //*/

    element test = {TYPE_BOOLEAN, "test", 1};
    element el1 = {TYPE_INT, "int1", 123};
    element el2 = {TYPE_INT, "int2", 100};
    condition* cond = condOr(condEqual(&el1), condLess(&el2));
    printf(checkCondition(&test, cond) ? "true\n" : "false\n");

    /*

    destroyDocumentRef(root1);
    destroyDocumentRef(root2);

    destroySchema(childSchema);
    destroySchema(grandChildSchema);
    destroySchema(root1Schema);
    destroySchema(root2Schema);

    */

    closeFile(file);
    return 0;
}