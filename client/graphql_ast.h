#ifndef _GRAPHQL_AST_H_
#define _GRAPHQL_AST_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum nodeType {
    SELECT_QUERY_NODE,
    INSERT_QUERY_NODE,
    UPDATE_QUERY_NODE,
    DELETE_QUERY_NODE,
    NESTED_QUERY_NODE,
    QUERY_SET_NODE, // узел списка запросов; в node->left - query, в node->right - указатель на следующий узел в списке
    OBJECT_NODE,
    VALUES_NODE,
    ELEMENT_SET_NODE, // узел списка элементов; в node->left - element,  в node->right - указатель на следующий узел в списке
    ELEMENT_NODE,
    KEY_NODE,
    INT_VAL_NODE,
    DOUBLE_VAL_NODE,
    BOOL_VAL_NODE,
    STR_VAL_NODE,
    FILTER_NODE,
    OP_EQ_NODE,
    OP_NEQ_NODE,
    OP_GT_NODE,
    OP_GTE_NODE,
    OP_LE_NODE,
    OP_LEE_NODE,
    OP_LIKE_NODE,
    OP_AND_NODE,
    OP_OR_NODE,
    OP_NOT_NODE,
} nodeType;

typedef struct astNode astNode;
typedef struct astNode {
    astNode* left;
    astNode* right;
    nodeType type;
    union {
        int32_t intVal;
        double doubleVal;
        bool boolVal;
        char* strVal;
    };
} astNode;

astNode* newNode();

astNode* newIntValNode(int32_t intVal);

astNode* newDoubleValNode(double doubleVal);

astNode* newBoolValNode(bool boolVal);

astNode* newStrValNode(char* strVal);

astNode* newKeyNode(char* key);

astNode* newElementNode(astNode* keyNode, astNode* valNode);

astNode* newElementSetNode(astNode* elementNode);

astNode* newValuesNode(astNode* elementSetNode);

astNode* newOperationNode(nodeType type, astNode* left, astNode* right);

astNode* newFilterNode(astNode* operationNode);

astNode* newObjectNode(char* schemaName, astNode* valuesNode, astNode* filterNode);

astNode* newQuerySetNode(astNode* queryNode);

astNode* newQueryNode(nodeType type, astNode* objectNode, astNode* querySetNode);

void addNextElementToSet(astNode* elementSetNode, astNode* nextElementSetNode);

void addNextQueryToSet(astNode* querySetNode, astNode* nextQuerySetNode);

void destroyNode(astNode* node);

void printNode(astNode* node, int32_t nestingLevel);

#endif