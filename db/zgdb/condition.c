#include <string.h>
#include <malloc.h>

#include "format.h"
#include "condition.h"
#include "element.h"
#include "schema.h"
#include "query.h"

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

condition* condEqual(element* el1, element* el2) {
    if (el1 && ((el1->type != TYPE_NOT_EXIST && !el2) ||
                (el1->type == TYPE_NOT_EXIST && el2 && el2->type == TYPE_NOT_EXIST))) {
        return createCondition(OP_EQ, el1, el2);
    }
    return NULL;
}

condition* condNotEqual(element* el1, element* el2) {
    if (el1 && ((el1->type != TYPE_NOT_EXIST && !el2) ||
                (el1->type == TYPE_NOT_EXIST && el2 && el2->type == TYPE_NOT_EXIST))) {
        return createCondition(OP_NEQ, el1, el2);
    }
    return NULL;
}

condition* condGreater(element* el1, element* el2) {
    if (el1 && ((el1->type != TYPE_NOT_EXIST && !el2) ||
                (el1->type == TYPE_NOT_EXIST && el2 && el2->type == TYPE_NOT_EXIST))) {
        return createCondition(OP_GT, el1, el2);
    }
    return NULL;
}

condition* condGreaterOrEqual(element* el1, element* el2) {
    if (el1 && ((el1->type != TYPE_NOT_EXIST && !el2) ||
                (el1->type == TYPE_NOT_EXIST && el2 && el2->type == TYPE_NOT_EXIST))) {
        return createCondition(OP_GTE, el1, el2);
    }
    return NULL;
}

condition* condLess(element* el1, element* el2) {
    if (el1 && ((el1->type != TYPE_NOT_EXIST && !el2) ||
                (el1->type == TYPE_NOT_EXIST && el2 && el2->type == TYPE_NOT_EXIST))) {
        return createCondition(OP_LE, el1, el2);
    }
    return NULL;
}

condition* condLessOrEqual(element* el1, element* el2) {
    if (el1 && ((el1->type != TYPE_NOT_EXIST && !el2) ||
                (el1->type == TYPE_NOT_EXIST && el2 && el2->type == TYPE_NOT_EXIST))) {
        return createCondition(OP_LEE, el1, el2);
    }
    return NULL;
}

condition* condLike(element* el1, element* el2) {
    if (el1 && ((el1->type != TYPE_NOT_EXIST && !el2) ||
                (el1->type == TYPE_NOT_EXIST && el2 && el2->type == TYPE_NOT_EXIST))) {
        return createCondition(OP_LIKE, el1, el2);
    }
    return NULL;
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

void destroyCondition(condition* cond) {
    if (cond) {
        if (cond->opType >= OP_AND) {
            destroyCondition(cond->cond1);
            destroyCondition(cond->cond2);
        } else {
            destroyElement(cond->el1);
            destroyElement(cond->el2);
        }
        free(cond);
    }
}

void resetCondition(condition* cond) {
    if (cond) {
        cond->isMet = false;
        if (cond->opType >= OP_AND) {
            // Для логических операций вызываем рекурсивный сброс:
            resetCondition(cond->cond1);
            resetCondition(cond->cond2);
        } else if (cond->el2) {
            // Если операция была над двумя элементами без значений, возвращаем их в изначальное состояние:
            cond->el1->type = TYPE_NOT_EXIST;
            cond->el2->type = TYPE_NOT_EXIST;
        }
    }
}

bool checkCondition(element* el, condition* cond) {
    // Если условие уже проверялось, то нет смысла проверять его ещё раз:
    if (cond->isMet) {
        return true;
    }

    // Для нелогических операций проводим ряд проверок:
    if (cond->opType < OP_AND) {
        if (cond->el2) {
            // Для операций над двумя ключами, подгружаем значения, если они ещё не были подгружены:
            if (cond->el1->type == TYPE_NOT_EXIST && strcmp(el->key, cond->el1->key) == 0) {
                *cond->el1 = *el;
            }
            if (cond->el2->type == TYPE_NOT_EXIST && strcmp(el->key, cond->el2->key) == 0) {
                *cond->el2 = *el;
            }
            // Если типы элементов не совпадают, то возвращаем false:
            if (cond->el1->type != cond->el2->type) {
                return false;
            }
        } else {
            // Если операция над одним элементом, то проверяем, совпадают ли тип и ключ:
            if (el->type != cond->el1->type || strcmp(el->key, cond->el1->key) != 0) {
                return false;
            }
        }
    }

    // Проверяем условие в зависимости от типа операции:
    bool result;
    switch (cond->opType) {
        case OP_EQ:
            result = (cond->el2 ? compare(cond->el1, cond->el2) : compare(el, cond->el1)) == 0;
            break;
        case OP_NEQ:
            result = (cond->el2 ? compare(cond->el1, cond->el2) : compare(el, cond->el1)) != 0;
            break;
        case OP_GT:
            result = (cond->el2 ? compare(cond->el1, cond->el2) : compare(el, cond->el1)) > 0;
            break;
        case OP_GTE:
            result = (cond->el2 ? compare(cond->el1, cond->el2) : compare(el, cond->el1)) >= 0;
            break;
        case OP_LE:
            result = (cond->el2 ? compare(cond->el1, cond->el2) : compare(el, cond->el1)) < 0;
            break;
        case OP_LEE:
            result = (cond->el2 ? compare(cond->el1, cond->el2) : compare(el, cond->el1)) <= 0;
            break;
        case OP_LIKE:
            result = cond->el2 ? strstr(cond->el1->stringValue.data, cond->el2->stringValue.data) :
                     strstr(el->stringValue.data, cond->el1->stringValue.data);
            break;
        case OP_AND:
            result = checkCondition(el, cond->cond1) & checkCondition(el, cond->cond2);
            break;
        case OP_OR:
            result = checkCondition(el, cond->cond1) | checkCondition(el, cond->cond2);
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

bool checkDocument(zgdbFile* file, uint64_t indexNumber, query* q) {
    // Если название схемы не указано, то это говорит о том, что документ ещё не создан, и проверять надо схему:
    if (q->type == INSERT_QUERY && !q->schemaName) {
        if (q->cond) {
            for (uint64_t i = 0; i < q->newValues->length; i++) {
                checkCondition(q->newValues->elements[i], q->cond);
            }
            return q->cond->isMet;
        }
        return true;
    }
    // Если документ существует, то проверяем его:
    document* doc = readDocument(file, indexNumber);
    if (doc) {
        if (!strcmp(q->schemaName, doc->header.schemaName)) {
            // Проверяем новые значения на соответствие схеме (если они переданы):
            if (q->newValues) {
                // Если хотя бы один ключ из списка новых элементов отсутствует в схеме документа, возвращаем false:
                for (uint64_t i = 0; i < q->newValues->length; i++) {
                    if (!getElementFromSchema(doc->schema, q->newValues->elements[i]->key)) {
                        destroyDocument(doc);
                        return false;
                    }
                }
            }
            // Проверяем условие:
            if (q->cond) {
                for (uint64_t i = 0; i < doc->schema->length; i++) {
                    checkCondition(doc->schema->elements[i], q->cond);
                }
            }
            destroyDocument(doc);
            // Если условие не передано, возвращаем true; иначе - результат проверки:
            return !q->cond || q->cond->isMet;
        }
        destroyDocument(doc);
    }
    return false;
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
    }
    return 0;
}