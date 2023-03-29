#include <glib-object.h>
#include <glib.h>
#include <stdio.h>

#include "graphql_ast.h"
#include "../gen-c_glib/structs_types.h"
#include "convertor.h"

astNode_t* convert(astNode* node) {
    printf("enter convert \n");
    if (node) {
        printf("11\n");
        value_t *val = g_object_new(TYPE_VALUE_T, NULL);
        printf("12\n");
        printf("type %d\n", node->type);
        switch (node->type) {
            case INT_VAL_NODE:
                printf("13\n");
                g_object_set(val, "intVal", node->intVal, NULL);
                printf("13\n");
                break;
            case DOUBLE_VAL_NODE:
                printf("17\n");
                g_object_set(val, "doubleVal", node->doubleVal, NULL);
                printf("17\n");
                break;
            case BOOL_VAL_NODE:
                printf("21\n");
                g_object_set(val, "boolVal", node->boolVal, NULL);
                printf("21\n");
                break;
            case KEY_NODE:
                g_object_set(val, "strVal", node->strVal, NULL);
                break;
            case OBJECT_NODE:
                g_object_set(val, "strVal", node->strVal, NULL);
                break;
            case JOIN_NODE:
                g_object_set(val, "strVal", node->strVal, NULL);
                break;
            case STR_VAL_NODE:
                printf("28\n");
                g_object_set(val, "strVal", node->strVal, NULL);
                printf("28\n");
                break;
        }
        printf("type %d\n", node->type);
        astNode_t *result = (astNode_t*) g_object_new(TYPE_AST_NODE_T, NULL);
        printf("33\n");
        g_object_set(result, "left", convert(node->left),
                             "right", convert(node->right),
                             "type", node->type,
                             "val", val, NULL);
        printf("38\n");
        return result;
    }
    return NULL;
}
