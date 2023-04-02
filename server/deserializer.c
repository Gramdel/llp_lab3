#include "deserializer.h"

element* deserializeElementNode(astNode_t* elementNode) {
    if (elementNode) {
        astNode_t* node1 = elementNode->left->pdata[0];
        astNode_t* node2 = elementNode->right->pdata[0];
        switch (node2->type) {
            case NODE_TYPE_T_KEY_NODE:
                // TODO: вызов конструктора для двух ключей
                break;
            case NODE_TYPE_T_FOREIGN_KEY_NODE:
                // TODO: вызов конструктора для ключа и чужого ключа
                break;
            case NODE_TYPE_T_INT_VAL_NODE:
                return intElement(node1->val->strVal, node2->val->intVal);
            case NODE_TYPE_T_DOUBLE_VAL_NODE:
                return doubleElement(node1->val->strVal, node2->val->doubleVal);
            case NODE_TYPE_T_BOOL_VAL_NODE:
                return booleanElement(node1->val->strVal, node2->val->boolVal);
            case NODE_TYPE_T_STR_VAL_NODE:
                // TODO: убрать кавычки
                return stringElement(node1->val->strVal, node2->val->strVal);
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
        switch (operationNode->type) {
            case NODE_TYPE_T_OP_EQ_NODE:
                return condEqual(deserializeElementNode(operationNode));
            case NODE_TYPE_T_OP_NEQ_NODE:
                return condNotEqual(deserializeElementNode(operationNode));
            case NODE_TYPE_T_OP_GT_NODE:
                return condGreater(deserializeElementNode(operationNode));
            case NODE_TYPE_T_OP_GTE_NODE:
                return condGreaterOrEqual(deserializeElementNode(operationNode));
            case NODE_TYPE_T_OP_LE_NODE:
                return condLess(deserializeElementNode(operationNode));
            case NODE_TYPE_T_OP_LEE_NODE:
                return condLessOrEqual(deserializeElementNode(operationNode));
            case NODE_TYPE_T_OP_LIKE_NODE:
                // TODO: like
                break;
            case NODE_TYPE_T_OP_AND_NODE:
                return condAnd(deserializeOperationNode(operationNode->left->pdata[0]),
                               deserializeOperationNode(operationNode->right->pdata[0]));
            case NODE_TYPE_T_OP_OR_NODE:
                return condOr(deserializeOperationNode(operationNode->left->pdata[0]),
                              deserializeOperationNode(operationNode->right->pdata[0]));
            case NODE_TYPE_T_OP_NOT_NODE:
                return condNot(deserializeOperationNode(operationNode->left->pdata[0]));
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