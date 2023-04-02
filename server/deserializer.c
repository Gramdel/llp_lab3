#include "deserializer.h"

element* deserializeElementNode(astNode_t* elementNode) {
    if (elementNode) {
        astNode_t* node1 = elementNode->left->pdata[0];
        astNode_t* node2 = elementNode->right->pdata[0];
        switch (node2->type) {
            case NODE_TYPE_T_INT_VAL_NODE:
                return intElement(node1->val->strVal, node2->val->intVal);
            case NODE_TYPE_T_DOUBLE_VAL_NODE:
                return doubleElement(node1->val->strVal, node2->val->doubleVal);
            case NODE_TYPE_T_BOOL_VAL_NODE:
                return booleanElement(node1->val->strVal, node2->val->boolVal);
            case NODE_TYPE_T_STR_VAL_NODE:
                return stringElement(node1->val->strVal,
                                     g_utf8_substring(node2->val->strVal, 1, (glong) strlen(node2->val->strVal) - 1));
        }
    }
    return NULL;
}

documentSchema* deserializeValuesNode(astNode_t* valuesNode, const char* schemaName) {
    if (valuesNode) {
        documentSchema* newElements = schemaName ? createSchema(schemaName) : createElements();
        astNode_t* elementSetNode = valuesNode->left->pdata[0];
        while (elementSetNode) {
            astNode_t* elementNode = elementSetNode->left->pdata[0];
            addElementToSchema(newElements, deserializeElementNode(elementNode));
            elementSetNode = elementSetNode->right->len ? elementSetNode->right->pdata[0] : NULL;
        }
        return newElements;
    }
    return NULL;
}

condition* deserializeOperationNode(astNode_t* operationNode) {
    if (operationNode) {
        astNode_t* left = operationNode->left->pdata[0];
        astNode_t* right = operationNode->right->pdata[0];
        switch (operationNode->type) {
            case NODE_TYPE_T_OP_EQ_NODE:
                if (right->type == NODE_TYPE_T_KEY_NODE) {
                    return condEqual(noValueElement(left->val->strVal), noValueElement(right->val->strVal));
                } else {
                    return condEqual(deserializeElementNode(operationNode), NULL);
                }
            case NODE_TYPE_T_OP_NEQ_NODE:
                if (right->type == NODE_TYPE_T_KEY_NODE) {
                    return condNotEqual(noValueElement(left->val->strVal), noValueElement(right->val->strVal));
                } else {
                    return condNotEqual(deserializeElementNode(operationNode), NULL);
                }
            case NODE_TYPE_T_OP_GT_NODE:
                if (right->type == NODE_TYPE_T_KEY_NODE) {
                    return condGreater(noValueElement(left->val->strVal), noValueElement(right->val->strVal));
                } else {
                    return condGreater(deserializeElementNode(operationNode), NULL);
                }
            case NODE_TYPE_T_OP_GTE_NODE:
                if (right->type == NODE_TYPE_T_KEY_NODE) {
                    return condGreaterOrEqual(noValueElement(left->val->strVal), noValueElement(right->val->strVal));
                } else {
                    return condGreaterOrEqual(deserializeElementNode(operationNode), NULL);
                }
            case NODE_TYPE_T_OP_LE_NODE:
                if (right->type == NODE_TYPE_T_KEY_NODE) {
                    return condLess(noValueElement(left->val->strVal), noValueElement(right->val->strVal));
                } else {
                    return condLess(deserializeElementNode(operationNode), NULL);
                }
            case NODE_TYPE_T_OP_LEE_NODE:
                if (right->type == NODE_TYPE_T_KEY_NODE) {
                    return condLessOrEqual(noValueElement(left->val->strVal), noValueElement(right->val->strVal));
                } else {
                    return condLessOrEqual(deserializeElementNode(operationNode), NULL);
                }
            case NODE_TYPE_T_OP_LIKE_NODE:
                if (right->type == NODE_TYPE_T_KEY_NODE) {
                    return condLike(noValueElement(left->val->strVal), noValueElement(right->val->strVal));
                } else {
                    return condLike(deserializeElementNode(operationNode), NULL);
                }
            case NODE_TYPE_T_OP_AND_NODE:
                return condAnd(deserializeOperationNode(left), deserializeOperationNode(right));
            case NODE_TYPE_T_OP_OR_NODE:
                return condOr(deserializeOperationNode(left), deserializeOperationNode(right));
            case NODE_TYPE_T_OP_NOT_NODE:
                return condNot(deserializeOperationNode(left));
        }
    }
    return NULL;
}

query* deserializeQueryNode(astNode_t* queryNode, zgdbFile* file) {
    if (queryNode) {
        astNode_t* objectNode = queryNode->left->pdata[0];
        astNode_t* valuesNode = objectNode->left->len ? objectNode->left->pdata[0] : NULL;
        astNode_t* filterNode = objectNode->right->len ? objectNode->right->pdata[0] : NULL;
        astNode_t* joinNode = filterNode && filterNode->left->len ? filterNode->left->pdata[0] : NULL;
        astNode_t* operationNode = filterNode && filterNode->right->len ? filterNode->right->pdata[0] : NULL;

        // Создаём запрос:
        query* q = NULL;
        switch (queryNode->type) {
            case NODE_TYPE_T_SELECT_QUERY_NODE:
                q = createSelectQuery(objectNode->val->strVal, deserializeOperationNode(operationNode));
                break;
            case NODE_TYPE_T_INSERT_QUERY_NODE: {
                documentSchema* schema = deserializeValuesNode(valuesNode, objectNode->val->strVal);
                q = createInsertQuery(!schema ? objectNode->val->strVal : NULL, schema,
                                      deserializeOperationNode(operationNode));
                break;
            }
            case NODE_TYPE_T_UPDATE_QUERY_NODE:
                q = createUpdateQuery(objectNode->val->strVal, deserializeValuesNode(valuesNode, NULL),
                                      deserializeOperationNode(operationNode));
                break;
            case NODE_TYPE_T_DELETE_QUERY_NODE:
                q = createDeleteQuery(objectNode->val->strVal, deserializeOperationNode(operationNode));
                break;
        }

        // Проходим по списку из querySetNode и добавляем вложенные запросы:
        if (q) {
            astNode_t* querySetNode = queryNode->right->len ? queryNode->right->pdata[0] : NULL;
            while (querySetNode) {
                queryNode = querySetNode->left->pdata[0];
                querySetNode = querySetNode->right->len ? querySetNode->right->pdata[0] : NULL;
                addNestedQuery(q, deserializeQueryNode(queryNode, file));
            }
            return q;
        }
    }
    return NULL;
}