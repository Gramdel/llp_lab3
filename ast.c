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

astNode* newElementNode(const char* name, astNode* valNode) {
    astNode* node = newNode();
    if (node) {
        node->type = ELEMENT_NODE;
        node->name = name;
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
        switch (node->type) {
            case SELECT_QUERY:
                break;
        }
    } else {
        printf("None");
    }
}