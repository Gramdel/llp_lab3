#include <string.h>

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
    /* Проверка на то, что само условие и его операнды не NULL (кроме операции "!", там второй операнд игнорируется).
     * Также, для НЕ логических операций, нужно, чтобы тип элементов совпадал. */
    if (!cond || !cond->condition1 || (cond->opType != OP_NOT && !cond->condition2) ||
        (cond->opType < OP_AND && cond->element1->type != cond->element2->type)) {
        return false;
    }

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