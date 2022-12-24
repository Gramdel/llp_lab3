#include <stdio.h>
#include <malloc.h>
#include "zgdb/document_public.h"

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
        addEmbeddedDocumentToSchema(file, childSchema, "grandChild", grandChildSchema);
    }

    documentSchema* root2Schema = createSchema(2); // size
    if (root2Schema) {
        addIntegerToSchema(root2Schema, "rootInt1", 123);
        addIntegerToSchema(root2Schema, "rootInt2", 456);
        addIntegerToSchema(root2Schema, "rootInt3", 789);
        addBooleanToSchema(root2Schema, "isFirst", true);
        addDoubleToSchema(root2Schema, "rootDouble", 128.128);
        addStringToSchema(root2Schema, "rootString", "I AM ROOT");
        addEmbeddedDocumentToSchema(file, root2Schema, "child", childSchema);
    }

    documentRef* root1 = writeDocument(file, root1Schema);
    documentRef* root2 = writeDocument(file, root2Schema);

    printDocument(file, root1);
    printDocument(file, root2);

    destroyDocumentRef(root1);
    destroyDocumentRef(root2);

    //element el = readElement(file, "grandChildSchema", readElement(file, "childSchema", root2Schema));

    /*/
    printf("removed? %d\n", removeDocument(file, 1));
    //printf("i: %d\n", writeDocument(file, root1Schema)); // 1


    //printf("updated? %d\n", updateIntegerValue(file, "fou2", 808, 2));
    //printf("updated? %d\n", updateBooleanValue(file, "isFirst", false, 2));
    //printf("updated? %d\n", updateDoubleValue(file, "testDouble", -2.5, 2));
    printf("updated? %d\n", updateStringValue(file, "testString", "I AM STRINGG", 2));
    //printf("updated? %d\n", updateDocumentValue(file, "testDoc", 1, 2));
    printf("i: %d\n", writeDocument(file, root2Schema)); // 9

    element el1 = readElement(file, "fou2", 2);
    element el2 = readElement(file, "isFirst", 2);
    element el3 = readElement(file, "testDouble", 2);
    element el4 = readElement(file, "testString", 2);
    element el5 = readElement(file, "testDoc", 2);

    printElement(el1);
    printElement(el2);
    printElement(el3);
    printElement(el4);
    printElement(el5);

    destroyElement(el4);
    //*/

    destroySchema(childSchema);
    destroySchema(grandChildSchema);
    destroySchema(root1Schema);
    destroySchema(root2Schema);

    closeFile(file);
    return 0;
}