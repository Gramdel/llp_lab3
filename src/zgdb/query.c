#include <string.h>
#include <malloc.h>

#include "query.h"
#include "element.h"

bool executeQuery(documentRef* parent, documentSchema* neededSchema, condition* cond) {
    // TODO: вызов функции find
    return true;
}

// TODO: подумать над тем, каким образом передавать новые значения полей
bool executeMutation(documentRef* parent, documentSchema* neededSchema, condition* cond) {
    // TODO: вызов функции find
    return true;
}

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

bool checkCondition(element* el, condition* cond) {
    // Если условие уже проверялось, то нет смысла проверять его ещё раз:
    if (cond->isMet) {
        return true;
    }
    // Если у элементов не совпадает тип или ключ, то нет смысла их сравнивать:
    if (cond->opType < OP_AND && (el->type != cond->el->type || strcmp(el->key, cond->el->key) != 0)) {
        return false;
    }
    // Проверяем условие в зависимости от типа операции:
    bool result;
    switch (cond->opType) {
        case OP_EQ:
            result = compare(el, cond->el) == 0;
            break;
        case OP_NEQ:
            result = compare(el, cond->el) != 0;
            break;
        case OP_GT:
            result = compare(el, cond->el) > 0;
            break;
        case OP_GTE:
            result = compare(el, cond->el) >= 0;
            break;
        case OP_LE:
            result = compare(el, cond->el) < 0;
            break;
        case OP_LEE:
            result = compare(el, cond->el) <= 0;
            break;
        case OP_AND:
            result = checkCondition(el, cond->cond1) && checkCondition(el, cond->cond2);
            break;
        case OP_OR:
            result = checkCondition(el, cond->cond1) || checkCondition(el, cond->cond2);
            break;
        case OP_NOT:
            result = !checkCondition(el, cond->cond1);
            break;
    }
    // Если условие выполнилось, то записываем в него эту информацию:
    if (result) {
        cond->isMet = true;
    }
    return result;
}

void resetCondition(condition* cond) {
    if (cond) {
        if (cond->opType < OP_AND) {
            cond->isMet = false;
        } else {
            resetCondition(cond->cond1);
            resetCondition(cond->cond2);
        }
    }
}

// PRIVATE
condition* createCondition(operationType type, void* operand1, void* operand2) {
    condition* cond = NULL;
    if ((cond = malloc(sizeof(condition)))) {
        cond->isMet = false;
        cond->opType = type;
        cond->cond1 = operand1;
        cond->cond2 = operand2;
    }
    return cond;
}

// PUBLIC
condition* condEqual(element* el) {
    return el ? createCondition(OP_EQ, el, NULL) : NULL;
}

condition* condNotEqual(element* el) {
    return el ? createCondition(OP_NEQ, el, NULL) : NULL;
}

condition* condGreater(element* el) {
    return el ? createCondition(OP_GT, el, NULL) : NULL;
}

condition* condGreaterOrEqual(element* el) {
    return el ? createCondition(OP_GTE, el, NULL) : NULL;
}

condition* condLess(element* el) {
    return el ? createCondition(OP_LE, el, NULL) : NULL;
}

condition* condLessOrEqual(element* el) {
    return el ? createCondition(OP_LEE, el, NULL) : NULL;
}

condition* condAnd(condition* cond1, condition* cond2) {
    return (cond1 && cond2) ? createCondition(OP_AND, cond1, cond2) : NULL;
}

condition* condOr(condition* cond1, condition* cond2) {
    return (cond1 && cond2) ? createCondition(OP_OR, cond1, cond2) : NULL;
}

condition* condNot(condition* cond) {
    return cond ? createCondition(OP_NOT, cond, NULL) : NULL;
}