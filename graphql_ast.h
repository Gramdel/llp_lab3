#ifndef _GRAPHQL_AST_H_
#define _GRAPHQL_AST_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum nodeType {
    SELECT_QUERY,
    INSERT_QUERY,
    UPDATE_QUERY,
    DELETE_QUERY,
    OBJECT_NODE,
    QUERY_SET_NODE,
    VALUES_NODE,
    ELEMENT_SET_NODE,
    ELEMENT_NODE,
    INT_NODE,
    DOUBLE_NODE,
    BOOL_NODE,
    STR_NODE,
    FILTER_NODE,
    OP_EQ,
    OP_NEQ,
    OP_GT,
    OP_GTE,
    OP_LE,
    OP_LEE,
    OP_LIKE,
    OP_AND,
    OP_OR,
    OP_NOT,
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
        const char* strVal;
    };
} astNode;

astNode* newNode();

astNode* newIntValNode(int32_t intVal);

astNode* newDoubleValNode(double doubleVal);

astNode* newBoolValNode(bool boolVal);

astNode* newStrValNode(const char* strVal);

astNode* newElementNode(astNode* strNode, astNode* valNode);

astNode* newElementSetNode(astNode* elementNode);

astNode* newValuesNode(astNode* elementSetNode);

astNode* newOperationNode(nodeType type, astNode* left, astNode* right);

astNode* newFilterNode(astNode* operationNode);

astNode* newObjectNode(const char* name, astNode* valuesNode, astNode* filterNode);

astNode* newQuerySetNode(astNode* queryNode);

astNode* newQueryNode(nodeType type, astNode* objectNode, astNode* querySetNode);

void addNextElementToSet(astNode* elementSetNode, astNode* nextElementSetNode);

void addNextQueryToSet(astNode* querySetNode, astNode* nextQuerySetNode);

void printNode(astNode* node);

#endif