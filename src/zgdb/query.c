#include <string.h>
#include <malloc.h>

#include "query.h"

int32_t compare(element* el1, element* el2) {
    switch (el1->type) {
        case TYPE_INT:
            return (el1->integerValue > el2->integerValue) - (el1->integerValue < el2->integerValue);
        case TYPE_DOUBLE:
            return (el1->doubleValue > el2->doubleValue) - (el1->doubleValue < el2->doubleValue);
        case TYPE_BOOLEAN:
            return el1->booleanValue - el2->booleanValue;
        case TYPE_STRING:
            return strcmp(el1->stringValue.data, el2->stringValue.data);
        case TYPE_EMBEDDED_DOCUMENT:
            // TODO: распарсить el2->schemaValue и проверить документ el1->documentValue на соответствие (поэлементно)
            return 0;
    }
    return 0;
}

bool checkCondition(condition* cond) {
    switch (cond->opType) {
        case OP_EQ:
            return compare(cond->element1, cond->element2) == 0;
        case OP_NEQ:
            return compare(cond->element1, cond->element2) != 0;
        case OP_GT:
            return compare(cond->element1, cond->element2) > 0;
        case OP_GTE:
            return compare(cond->element1, cond->element2) >= 0;
        case OP_LE:
            return compare(cond->element1, cond->element2) < 0;
        case OP_LEE:
            return compare(cond->element1, cond->element2) <= 0;
        case OP_AND:
            return checkCondition(cond->condition1) && checkCondition(cond->condition2);
        case OP_OR:
            return checkCondition(cond->condition1) || checkCondition(cond->condition2);
        case OP_NOT:
            return !checkCondition(cond->condition1);
    }
}

condition* createSimpleCondition(operationType type, element* el1, element* el2) {
    condition* cond = NULL;
    if (type < 6 && el1 && el2 && (el1->type == el2->type) && (cond = malloc(sizeof(condition)))) {
        cond->opType = type;
        cond->element1 = el1;
        cond->element2 = el2;
    }
    return cond;
}

condition* createComplexCondition(operationType type, condition* cond1, condition* cond2) {
    condition* cond = NULL;
    if (type >= 6 && cond1 && cond2 && (cond = malloc(sizeof(condition)))) {
        cond->opType = type;
        cond->condition1 = cond1;
        cond->condition2 = cond2;
    }
    return cond;
}