#include <string.h>
#include <malloc.h>

#include "format.h"
#include "condition.h"
#include "element.h"
#include "schema.h"

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

void resetCondition(condition* cond) {
    if (cond) {
        cond->isMet = false;
        if (cond->opType >= OP_AND) {
            resetCondition(cond->cond1);
            resetCondition(cond->cond2);
        }
    }
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

bool checkDocument(zgdbFile* file, uint64_t indexNumber, const char* schemaName, condition* cond) {
    // Если название схемы не указано, то это говорит о том, что документ ещё не создан. Возвращаем true:
    if (!schemaName) {
        return true;
    }
    // Если название указано, то находим индекс и проверяем документ:
    if (indexNumber != DOCUMENT_NOT_EXIST) {
        zgdbIndex index = getIndex(file, indexNumber);
        if (index.flag == INDEX_ALIVE) {
            fseeko64(file->f, index.offset, SEEK_SET); // спуск в документ по смещению
            documentHeader header;
            // Читаем заголовок документа и проверяем, соответствует ли имя его схемы имени требуемой:
            if (fread(&header, sizeof(documentHeader), 1, file->f) && !strcmp(schemaName, header.schemaName)) {
                // Если условие не указано, то соответствия схемы достаточно:
                if (!cond) {
                    return true;
                }
                uint64_t bytesRead = sizeof(documentHeader);
                while (bytesRead < header.size) {
                    element el;
                    uint64_t tmp = readElement(file, &el, false);
                    if (!tmp) {
                        return false;
                    }
                    // Если был достигнут конец документа (пустое место в нём), то выходим из цикла:
                    if (el.type == TYPE_NOT_EXIST) {
                        bytesRead = header.size;
                    } else {
                        bytesRead += tmp;
                        checkCondition(&el, cond);
                    }
                }
                return cond->isMet;
            }
        }
    }
    return false;
}