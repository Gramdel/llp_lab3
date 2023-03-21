#include <string.h>
#include <malloc.h>

#include "format.h"
#include "query.h"
#include "condition.h"
#include "element.h"
#include "iterator.h"
#include "schema.h"

query* createQuery(queryType type, const char* schemaName, documentSchema* newValues, condition* cond) {
    query* q = malloc(sizeof(query));
    if (q) {
        q->type = type;
        q->schemaName = schemaName;
        q->newValues = newValues;
        q->cond = cond;
        q->nestedQueries = NULL;
        q->length = 0;
        return q;
    }
    return NULL;
}

query* createSelectQuery(const char* schemaName, condition* cond) {
    if (schemaName && strlen(schemaName) <= 12) {
        return createQuery(SELECT_QUERY, schemaName, NULL, cond);
    }
    return NULL;
}

query* createInsertQuery(const char* schemaName, documentSchema* newValues, condition* cond) {
    /* Если сам query (а не его будущие вложенные query) должен добавить новый документ, то добавление должно произойти
     * без условий, ну и должна быть передана схема документа: */
    if (!schemaName && (!newValues || cond)) {
        return NULL;
    }
    /* Если сам query используется для навигации по дереву, то в него нельзя передать схему и его имя не должно быть
     * длиннее 12 символов: */
    if (schemaName && (newValues || strlen(schemaName) > 12)) {
        return NULL;
    }
    return createQuery(INSERT_QUERY, schemaName, newValues, cond);
}

query* createUpdateQuery(const char* schemaName, documentSchema* newValues, condition* cond) {
    if (schemaName && strlen(schemaName) <= 12) {
        return createQuery(UPDATE_QUERY, schemaName, newValues, cond);
    }
    return NULL;
}

query* createDeleteQuery(const char* schemaName, condition* cond) {
    if (schemaName && strlen(schemaName) <= 12) {
        return createQuery(DELETE_QUERY, schemaName, NULL, cond);
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

bool addNestedQuery(query* q, query* nq) {
    if (q && nq && q->type == nq->type) {
        /* В случае, если мы добавляем insert/update запрос, который находится на самом нижнем уровне, и не передаём туда
         * значения для записи, то это ошибка: */
        if ((nq->type == INSERT_QUERY || nq->type == UPDATE_QUERY) && !nq->newValues && !nq->nestedQueries) {
            return false;
        }
        query** tmp = realloc(q->nestedQueries, sizeof(query*) * (q->length + 1));
        if (tmp) {
            q->nestedQueries = tmp;
            q->nestedQueries[q->length++] = nq;
            return true;
        }
    }
    return false;
}

iterator* executeSelect(zgdbFile* file, query* q) {
    if (q && q->type == SELECT_QUERY) {
        iterator* it = createIterator();
        if (it) {
            if (find(file, it, file->header.indexOfRoot, q, NULL)) {
                return it;
            }
            destroyIterator(it);
        }
    }
    return NULL;
}

bool executeInsert(zgdbFile* file, query* q) {
    // Помимо обычных проверок, проверяем, что не создаём второй корень:
    if (q && q->type == INSERT_QUERY && (q->schemaName || file->header.indexOfRoot == DOCUMENT_NOT_EXIST)) {
        return find(file, NULL, file->header.indexOfRoot, q, &insertDocument);
    }
    return false;
}

bool executeUpdate(zgdbFile* file, query* q) {
    if (q && q->type == UPDATE_QUERY) {
        return find(file, NULL, file->header.indexOfRoot, q, &updateDocument);
    }
    return false;
}

bool executeDelete(zgdbFile* file, query* q) {
    if (q && q->type == DELETE_QUERY) {
        return findAndMutate(file, NULL, NULL, file->header.indexOfRoot, q, &removeDocument);
    }
    return false;
}

bool find(zgdbFile* file, iterator* it, uint64_t indexNumber, query* q, bool (* mutate)(zgdbFile*, uint64_t*, query*)) {
    bool match = checkDocument(file, indexNumber, q->schemaName, q->cond);
    resetCondition(q->cond); // сброс cond.met
    if (match && (!mutate || mutate(file, &indexNumber, q))) {
        // Если выше было вызвано рекурсивное добавление новых документов, то всё уже добавлено. Можно выходить:
        if (!q->schemaName) {
            return true;
        }
        // Если есть вложенные query, продолжаем рекурсию; иначе - добавляем в итератор текущий индекс (в случае select):
        if (q->nestedQueries) {
            zgdbIndex index = getIndex(file, indexNumber);
            if (index.flag == INDEX_ALIVE) {
                fseeko64(file->f, index.offset, SEEK_SET);
                documentHeader header;
                if (fread(&header, sizeof(documentHeader), 1, file->f)) {
                    uint64_t childIndexNumber = header.lastChildIndexNumber;
                    while (childIndexNumber != DOCUMENT_NOT_EXIST) {
                        // Запускаем рекурсивную проверку вложенных условий:
                        for (uint64_t i = 0; i < q->length; i++) {
                            if (!find(file, it, childIndexNumber, q->nestedQueries[i], mutate)) {
                                return false;
                            }
                        }
                        // Выбираем следующего ребёнка:
                        zgdbIndex childIndex = getIndex(file, childIndexNumber);
                        if (childIndex.flag == INDEX_ALIVE) {
                            fseeko64(file->f, childIndex.offset, SEEK_SET); // спуск в ребёнка
                            documentHeader childHeader;
                            if (fread(&childHeader, sizeof(documentHeader), 1, file->f)) {
                                childIndexNumber = childHeader.brotherIndexNumber;
                                continue;
                            }
                        }
                        return false;
                    }
                    return true;
                }
            }
        } else if (!mutate) {
            return addRef(it, indexNumber);
        }
        return true;
    }
    return false;
}

bool findAndMutate(zgdbFile* file, iterator* it, documentHeader* parentHeader, uint64_t indexNumber, query* q,
                   bool (* mutate)(zgdbFile*, documentHeader*, uint64_t*, query*)) {
    bool match = checkDocument(file, indexNumber, q->schemaName, q->cond);
    resetCondition(q->cond); // сброс cond.met
    if (match && (!mutate || mutate(file, parentHeader, &indexNumber, q))) {
        // Если выше было вызвано рекурсивное добавление новых документов, то всё уже добавлено. Можно выходить:
        if (!q->schemaName) {
            return true;
        }
        // Если есть вложенные query, продолжаем рекурсию; иначе - добавляем в итератор текущий индекс (в случае select):
        if (q->nestedQueries) {
            zgdbIndex index = getIndex(file, indexNumber);
            if (index.flag == INDEX_ALIVE) {
                fseeko64(file->f, index.offset, SEEK_SET);
                documentHeader header;
                if (fread(&header, sizeof(documentHeader), 1, file->f)) {
                    uint64_t childIndexNumber = header.lastChildIndexNumber;
                    while (childIndexNumber != DOCUMENT_NOT_EXIST) {
                        // Выбираем следующего ребёнка (заранее, поскольку текущий ребёнок может удалиться):
                        zgdbIndex childIndex = getIndex(file, childIndexNumber);
                        if (childIndex.flag == INDEX_ALIVE) {
                            fseeko64(file->f, childIndex.offset, SEEK_SET);
                            documentHeader childHeader;
                            if (fread(&childHeader, sizeof(documentHeader), 1, file->f)) {
                                // Запускаем рекурсивную проверку вложенных условий:
                                for (uint64_t i = 0; i < q->length; i++) {
                                    if (!findAndMutate(file, it, &childHeader, childIndexNumber, q->nestedQueries[i],
                                                       mutate)) {
                                        return false;
                                    }
                                }
                                childIndexNumber = childHeader.brotherIndexNumber;
                                continue;
                            }
                        }
                        return false;
                    }
                    return true; // TODO: может быть это не всегда true
                }
            }
        } else if (!mutate) {
            return addRef(it, indexNumber);
        }
        return true;
    }
    return false;
}