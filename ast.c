#include <malloc.h>
#include <stdio.h>
#include "ast.h"

astNode* newNode() {
    astNode* node = malloc(sizeof(astNode));
    if (node) {
        *node = (astNode) { 0 };
        return node;
    }
    return NULL;
}

astNode* newIntValNode(int32_t intVal) {
    astNode* node = newNode();
    if (node) {
        node->type = INT_NODE;
        node->intVal = intVal;
    }
    return node;
}

astNode* newDoubleValNode(double doubleVal) {
    astNode* node = newNode();
    if (node) {
        node->type = DOUBLE_NODE;
        node->doubleVal = doubleVal;
    }
    return node;
}

astNode* newBoolValNode(bool boolVal) {
    astNode* node = newNode();
    if (node) {
        node->type = BOOL_NODE;
        node->boolVal = boolVal;
    }
    return node;
}

astNode* newStrValNode(const char* strVal) {
    astNode* node = newNode();
    if (node) {
        node->type = STR_NODE;
        node->strVal = strVal;
    }
    return node;
}

astNode* newElementNode(astNode* strNode, astNode* valNode) {
    astNode* node = newNode();
    if (node) {
        node->type = ELEMENT_NODE;
        node->left = strNode;
        node->left = valNode;
    }
    return node;
}

astNode* newElementSetNode(astNode* elementNode, astNode* nextElementSetNode) {
    astNode* node = newNode();
    if (node) {
        node->type = ELEMENT_SET_NODE;
        node->left = elementNode;
        node->right = nextElementSetNode;
    }
    return node;
}

astNode* newValuesNode(astNode* elementSetNode) {
    astNode* node = newNode();
    if (node) {
        node->type = VALUES_NODE;
        node->left = elementSetNode;
    }
    return node;
}

astNode* newOperationNode(nodeType type, astNode* left, astNode* right) {
    astNode* node = newNode();
    if (node) {
        node->type = type;
        node->left = left;
        node->left = right;
    }
    return node;
}

astNode* newFilterNode(astNode* operationNode) {
    astNode* node = newNode();
    if (node) {
        node->type = FILTER_NODE
        node->left = operationNode;
    }
    return node;
}

astNode* newObjectNode(const char* name, astNode* valuesNode, astNode* filterNode) {
    astNode* node = newNode();
    if (node) {
        node->type = OBJECT_NODE;
        node->name = name;
        node->left = valuesNode;
        node->right = filterNode;
    }
    return node;
}

astNode* newObjectSetNode(astNode* ObjectNode, astNode* nextObjectSetNode) {
    astNode* node = newNode();
    if (node) {
        node->type = OBJECT_SET_NODE;
        node->left = firstObjectNode;
        node->right = nextObjectSetNode;
    }
    return node;
}

astNode* newQueryNode(nodeType type, astNode* objectSetNode) {
    astNode* node = newNode();
    if (node) {
        node->type = type;
        node->left = objectSetNode;
    }
    return node;
}

void printNode(astNode* node) {
    if (node) {
        if (node->type <= DELETE_QUERY) {
            printf("Query type: ");
            switch (node->type) {
                case SELECT_QUERY:
                    printf("Select\n");
                    break;
                case INSERT_QUERY:
                    printf("Insert\n");
                    break;
                case UPDATE_QUERY:
                    printf("Update\n");
                    break;
                case DELETE_QUERY:
                    printf("Delete\n");
                    break;
            }
            printf("ObjectSet: ");
            printNode(node->left);
        } else if (node->type <= FILTER_NODE) {
            switch (node->type) {
                case OBJECT_NODE:
                    printf("SchemaName: %s\n", node->strVal);
                    printf("NewValues: ");
                    printNode(node->left);
                    printf("Filter: ");
                    printNode(node->right);
                    break;
                case OBJECT_SET_NODE:
                    for (uint64_t i = 1; node->right; node = node->right, i++) {
                        printf("Object%lu:\n", i);
                        printNode(node->left);
                    }
                    break;
                case VALUES_NODE:
                    printf("ElementSet:\n");
                    printNode(node->left);
                    break;
                case ELEMENT_SET_NODE:
                    for (uint64_t i = 1; node->right; node = node->right, i++) {
                        printf("Element%lu:\n", i);
                        printNode(node->left);
                    }
                    break;
                case ELEMENT_NODE:
                    printf("Key: %s\n", node->left->strVal);
                    printf("Value:\n");
                    printNode(node->right);
                    break;
                case INT_NODE:
                    printf("ValueType: Integer\n");
                    printf("Data: %d\n", node->intVal);
                    break;
                case DOUBLE_NODE:
                    printf("ValueType: Double\n");
                    printf("Data: %f\n", node->doubleVal);
                    break;
                case BOOL_NODE:
                    printf("ValueType: Boolean\n");
                    printf("Data: %s\n", node->boolVal ? "true" : "false");
                    break;
                case STR_NODE:
                    printf("ValueType: String\n");
                    printf("Data: %s\n", node->strVal);
                    break;
                case FILTER_NODE:
                    printf("Operation:\n");
                    printNode(node->left);
                    break;
            }
        } else {
            printf("OperationType: ");
            switch (node->type) {
                case OP_EQ:
                    printf("Equal\n");
                    break;
                case OP_NEQ:
                    printf("NotEqual\n");
                    break;
                case OP_GT:
                    printf("Greater\n");
                    break;
                case OP_GTE:
                    printf("GreaterOrEqual\n");
                    break;
                case OP_LE:
                    printf("Less\n");
                    break;
                case OP_LEE:
                    printf("LessOrEqual\n");
                    break;
                case OP_LIKE:
                    printf("Like\n");
                    break;
                case OP_AND:
                    printf("And\n");
                    break;
                case OP_OR:
                    printf("Or\n");
                    break;
                case OP_NOT:
                    printf("Not\n");
                    break;
            }
            if (node->type <= OP_LIKE) {
                printf("Key: %s\n", node->left->strVal);
                printf("Value:\n");
                printNode(node->right);
            } else if (node->type <= OP_OR) {
                printf("Operation1:\n");
                printNode(node->left);
                printf("Operation2:\n");
                printNode(node->right);
            } else {
                printf("Operation:\n");
                printNode(node->left);
            }
        }
    } else {
        printf("None\n");
    }
}