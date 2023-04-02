#include <glib-object.h>
#include <glib.h>
#include <stdio.h>

#include "graphql_ast.h"
#include "serializer.h"

astNode_t* serialize(nodeType* parentType, astNode* node) {
    if (node) {
        nodeType* nextParentType = parentType;
        value_t* val = g_object_new(TYPE_VALUE_T, NULL);
        switch (node->type) {
            case SELECT_QUERY_NODE:
            case INSERT_QUERY_NODE:
            case UPDATE_QUERY_NODE:
            case DELETE_QUERY_NODE:
                nextParentType = &node->type;
                break;
            case NESTED_QUERY_NODE:
                node->type = *parentType;
                break;
            case INT_VAL_NODE:
                g_object_set(val, "intVal", node->intVal, NULL);
                break;
            case DOUBLE_VAL_NODE:
                g_object_set(val, "doubleVal", node->doubleVal, NULL);
                break;
            case BOOL_VAL_NODE:
                g_object_set(val, "boolVal", node->boolVal, NULL);
                break;
            case KEY_NODE:
            case OBJECT_NODE:
            case JOIN_NODE:
            case STR_VAL_NODE:
                g_object_set(val, "strVal", node->strVal, NULL);
                break;
        }
        // Рекурсивно вызываем функцию на node->left и node->right, если они не NULL:
        GPtrArray* left = g_ptr_array_new();
        if (node->left) {
            g_ptr_array_add(left, serialize(nextParentType, node->left));
        }
        GPtrArray* right = g_ptr_array_new();
        if (node->right) {
            g_ptr_array_add(right, serialize(nextParentType, node->right));
        }
        astNode_t* result = (astNode_t*) g_object_new(TYPE_AST_NODE_T, "left", left, "right", right, "type", node->type,
                                                      "val", val, NULL);
        return result;
    }
    return NULL;
}
