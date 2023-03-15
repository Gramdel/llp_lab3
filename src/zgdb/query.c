#include <string.h>
#include <malloc.h>

#include "query.h"
#include "element.h"
#include "iterator.h"
#include "schema.h"

query* createQuery(char* schemaName, condition* cond, uint64_t length, ...) {
    if (schemaName && strlen(schemaName) <= 12) {
        query* q = malloc(sizeof(query));
        if (q) {
            q->length = length;
            strcpy(q->schemaName, schemaName);
            q->cond = cond;
            if (length) {
                q->nestedQueries = malloc(sizeof(query*) * length);
                if (q->nestedQueries) {
                    va_list arg;
                    va_start(arg, length);
                    for (uint64_t i = 0; i < length; i++) {
                        q->nestedQueries[i] = va_arg(arg, query*);
                        if (!q->nestedQueries[i]) {
                            destroyQuery(q);
                            return NULL;
                        }
                    }
                    va_end(arg);
                    return q;
                }
                free(q);
            } else {
                q->nestedQueries = NULL;
                return q;
            }
        }
    }
    return NULL;
}

void destroyQuery(query* q) {
    if (q) {
        if (q->nestedQueries) {
            for (uint64_t i = 0; i < q->length; i++) {
                destroyQuery(q->nestedQueries[i]);
            }
            free(q->nestedQueries);
        }
        free(q);
    }
}

iterator* executeQuery(zgdbFile* file, query* q) {
    if (file->header.indexOfRoot != DOCUMENT_NOT_EXIST && q) {
        if (q->length) {
            zgdbIndex index = getIndex(file, file->header.indexOfRoot);
            if (index.flag == INDEX_ALIVE) {
                fseeko64(file->f, index.offset, SEEK_SET); // спуск в корень по смещению
                documentHeader header;
                /* Читаем заголовок корня и проверяем, соответствует ли имя корня в запросе его истинному имени. Если всё
                 * хорошо, то можно начинать поиск: */
                if (fread(&header, sizeof(documentHeader), 1, file->f) && !strcmp(q->schemaName, header.schemaName)) {
                    return findAllDocuments(file, file->header.indexOfRoot, q);
                }
            }
        } else {
            iterator* it = createIterator();
            if (it) {
                if (checkDocument(file, file->header.indexOfRoot, q->schemaName, q->cond) &&
                    !addRef(it, (documentRef) { file->header.indexOfRoot })) {
                    destroyIterator(it);
                    return NULL;
                }
                return it;
            }
        }
    }
    return NULL;
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
        cond->isMet = false;
        if (cond->opType >= OP_AND) {
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

bool checkDocument(zgdbFile* file, uint64_t indexNumber, const char* schemaName, condition* cond) {
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

iterator* findAllDocuments(zgdbFile* file, uint64_t parentIndexNumber, query* q) {
    zgdbIndex index = getIndex(file, parentIndexNumber);
    if (index.flag == INDEX_ALIVE) {
        fseeko64(file->f, index.offset, SEEK_SET); // спуск в родительский документ по смещению
        documentHeader header;
        if (fread(&header, sizeof(documentHeader), 1, file->f)) {
            iterator* it = createIterator();
            if (it) {
                uint64_t bytesRead = sizeof(documentHeader);
                while (bytesRead < header.size) {
                    element el;
                    uint64_t tmp = readElement(file, &el, true);
                    if (!tmp) {
                        destroyIterator(it);
                        return NULL;
                    }
                    switch (el.type) {
                        case TYPE_NOT_EXIST:
                            bytesRead = header.size;
                            break;
                        case TYPE_EMBEDDED_DOCUMENT:
                            // Проверяем документ и пытаемся добавить его в итератор (при удаче):
                            for (uint64_t i = 0; i < q->length; i++) {
                                query* nq = q->nestedQueries[i];
                                bool match = checkDocument(file, el.documentValue.indexNumber, nq->schemaName, nq->cond);
                                resetCondition(nq->cond); // сброс cond.met
                                if (match) {
                                    if (nq->length) {
                                        if (!addAllRefs(it,
                                                        findAllDocuments(file, el.documentValue.indexNumber, nq))) {
                                            destroyIterator(it);
                                            return NULL;
                                        }
                                    } else if (!addRef(it, el.documentValue)) {
                                        destroyIterator(it);
                                        return NULL;
                                    }
                                }

                            }
                        default:
                            bytesRead += tmp;
                    }
                }
                return it;
            }
        }
    }
    return NULL;
}