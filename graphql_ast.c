#include <malloc.h>
#include <stdio.h>
#include "graphql_ast.h"

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
        node->right = valNode;
    }
    return node;
}

astNode* newElementSetNode(astNode* elementNode) {
    astNode* node = newNode();
    if (node) {
        node->type = ELEMENT_SET_NODE;
        node->left = elementNode;
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
        node->right = right;
    }
    return node;
}

astNode* newFilterNode(astNode* operationNode) {
    astNode* node = newNode();
    if (node) {
        node->type = FILTER_NODE;
        node->left = operationNode;
    }
    return node;
}

astNode* newObjectNode(const char* name, astNode* valuesNode, astNode* filterNode) {
    astNode* node = newNode();
    if (node) {
        node->type = OBJECT_NODE;
        node->strVal = name;
        node->left = valuesNode;
        node->right = filterNode;
    }
    return node;
}

astNode* newQuerySetNode(astNode* queryNode, astNode* nextQuerySetNode) {
    astNode* node = newNode();
    if (node) {
        node->type = QUERY_SET_NODE;
        node->left = queryNode;
        node->right = nextQuerySetNode;
    }
    return node;
}

astNode* newQueryNode(nodeType type, astNode* objectNode, astNode* querySetNode) {
    astNode* node = newNode();
    if (node) {
        node->type = type;
        node->left = objectNode;
        node->right = querySetNode;
    }
    return node;
}

void addNextElementToSet(astNode* elementSetNode, astNode* nextElementSetNode) {
    if (elementSetNode) {
        elementSetNode->right = nextElementSetNode;
    }
}

void addNextQueryToSet(astNode* querySetNode, astNode* nextQuerySetNode) {
    if (querySetNode) {
        querySetNode->right = nextQuerySetNode;
    }
}

void printNode(astNode* node, int32_t nestingLevel) {
    if (node) {
        switch (node->type) {
            case SELECT_QUERY:
                printf("QueryType: Select\n");
                printf("QuerySet: ");
                printNode(node->right, nestingLevel + 2);
                break;
            case INSERT_QUERY:
                printf("QueryType: Insert\n");
                printf("QuerySet: ");
                printNode(node->right, nestingLevel + 2);
                break;
            case UPDATE_QUERY:
                printf("QueryType: Update\n");
                printf("QuerySet: ");
                printNode(node->right, nestingLevel + 2);
                break;
            case DELETE_QUERY:
                printf("QueryType: Delete\n");
                printf("QuerySet: ");
                printNode(node->right, nestingLevel + 2);
                break;
            case NESTED_QUERY:
                printf("%*sObject:\n", nestingLevel, "");
                printNode(node->left, nestingLevel + 2);
                printf("%*sQuerySet: ", nestingLevel, "");
                printNode(node->right, nestingLevel + 2);
                break;
            case QUERY_SET_NODE:
                printf("\n");
                while (node) {
                    printf("%*sQuery:\n", nestingLevel, "");
                    printNode(node->left, nestingLevel + 2);
                    node = node->right;
                }
                break;
            case OBJECT_NODE:
                printf("%*sSchemaName: %s\n", nestingLevel, "", node->strVal);
                printf("%*sNewValues: ", nestingLevel, "");
                printNode(node->left, nestingLevel + 2);
                printf("%*sFilter: ", nestingLevel, "");
                printNode(node->right, nestingLevel + 2);
                break;
            case VALUES_NODE:
                printf("\n%*sElementSet:\n", nestingLevel, "");
                printNode(node->left, nestingLevel + 2);
                break;
            case ELEMENT_SET_NODE:
                while (node) {
                    printf("%*sElement:\n", nestingLevel, "");
                    printNode(node->left, nestingLevel + 2);
                    node = node->right;
                }
                break;
            case ELEMENT_NODE:
                printf("%*sKey: %s\n", nestingLevel, "", node->left->strVal);
                printf("%*sValue:\n", nestingLevel, "");
                printNode(node->right, nestingLevel + 2);
                break;
            case INT_NODE:
                printf("%*sValueType: Integer\n", nestingLevel, "");
                printf("%*sData: %d\n", nestingLevel, "", node->intVal);
                break;
            case DOUBLE_NODE:
                printf("%*sValueType: Double\n", nestingLevel, "");
                printf("%*sData: %f\n", nestingLevel, "", node->doubleVal);
                break;
            case BOOL_NODE:
                printf("%*sValueType: Boolean\n", nestingLevel, "");
                printf("%*sData: %s\n", nestingLevel, "", node->boolVal ? "true" : "false");
                break;
            case STR_NODE:
                printf("%*sValueType: String\n", nestingLevel, "");
                printf("%*sData: %s\n", nestingLevel, "", node->strVal);
                break;
            case FILTER_NODE:
                printf("\n%*sOperation:\n", nestingLevel, "");
                printNode(node->left, nestingLevel + 2);
                break;
            case OP_EQ:
                printf("%*sOperationType: Equal\n", nestingLevel, "");
                printf("%*sKey: %s\n", nestingLevel, "", node->left->strVal);
                printf("%*sValue:\n", nestingLevel, "");
                printNode(node->right, nestingLevel + 2);
                break;
            case OP_NEQ:
                printf("%*sOperationType: NotEqual\n", nestingLevel, "");
                printf("%*sKey: %s\n", nestingLevel, "", node->left->strVal);
                printf("%*sValue:\n", nestingLevel, "");
                printNode(node->right, nestingLevel + 2);
                break;
            case OP_GT:
                printf("%*sOperationType: Greater\n", nestingLevel, "");
                printf("%*sKey: %s\n", nestingLevel, "", node->left->strVal);
                printf("%*sValue:\n", nestingLevel, "");
                printNode(node->right, nestingLevel + 2);
                break;
            case OP_GTE:
                printf("%*sOperationType: GreaterOrEqual\n", nestingLevel, "");
                printf("%*sKey: %s\n", nestingLevel, "", node->left->strVal);
                printf("%*sValue:\n", nestingLevel, "");
                printNode(node->right, nestingLevel + 2);
                break;
            case OP_LE:
                printf("%*sOperationType: Less\n", nestingLevel, "");
                printf("%*sKey: %s\n", nestingLevel, "", node->left->strVal);
                printf("%*sValue:\n", nestingLevel, "");
                printNode(node->right, nestingLevel + 2);
                break;
            case OP_LEE:
                printf("%*sOperationType: LessOrEqual\n", nestingLevel, "");
                printf("%*sKey: %s\n", nestingLevel, "", node->left->strVal);
                printf("%*sValue:\n", nestingLevel, "");
                printNode(node->right, nestingLevel + 2);
                break;
            case OP_LIKE:
                printf("%*sOperationType: Like\n", nestingLevel, "");
                printf("%*sKey: %s\n", nestingLevel, "", node->left->strVal);
                printf("%*sValue:\n", nestingLevel, "");
                printNode(node->right, nestingLevel + 2);
                break;
            case OP_AND:
                printf("%*sOperationType: And\n", nestingLevel, "");
                printf("%*sOperation1:\n", nestingLevel, "");
                printNode(node->left, nestingLevel + 2);
                printf("%*sOperation2:\n", nestingLevel, "");
                printNode(node->right, nestingLevel + 2);
                break;
            case OP_OR:
                printf("%*sOperationType: Or\n", nestingLevel, "");
                printf("%*sOperation1:\n", nestingLevel, "");
                printNode(node->left, nestingLevel + 2);
                printf("%*sOperation2:\n", nestingLevel, "");
                printNode(node->right, nestingLevel + 2);
                break;
            case OP_NOT:
                printf("%*sOperationType: Not\n", nestingLevel, "");
                printf("%*sOperation:\n", nestingLevel, "");
                printNode(node->left, nestingLevel + 2);
                break;
        }
    } else {
        printf("None\n");
    }
}