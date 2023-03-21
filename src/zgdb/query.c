#include <string.h>
#include <malloc.h>
#include <stdarg.h>

#include "query.h"
#include "condition.h"
#include "element.h"
#include "iterator.h"
#include "schema.h"

query* createQuery(const char* schemaName, condition* cond, queryType type, documentSchema* newValues, uint64_t length,
                   va_list arg) {
    query* q = malloc(sizeof(query));
    if (q) {
        q->schemaName = schemaName;
        q->cond = cond;
        q->type = type;
        q->newValues = newValues;
        if (length) {
            q->nestedQueries = malloc(sizeof(query*) * length);
            if (q->nestedQueries) {
                for (uint64_t i = 0; i < length; i++) {
                    q->length = i + 1;
                    q->nestedQueries[i] = va_arg(arg, query*);
                    // Если в качестве вложенного query передан NULL или тип q и nq не совпадает, то возвращаем NULL:
                    if (!q->nestedQueries[i] || q->nestedQueries[i]->type != q->type) {
                        destroyQuery(q);
                        return NULL;
                    }
                }
                return q;
            }
            free(q);
        } else {
            q->length = 0;
            q->nestedQueries = NULL;
            // Если запрос последний в цепочке, обновляющий, и newValues не переданы, то это неправильный запрос:
            if (q->type == SELECT_QUERY || q->type == DELETE_QUERY || q->newValues) {
                return q;
            }
            destroyQuery(q);
        }
    }
    return NULL;
}

query* insertQuery(const char* schemaName, condition* cond, documentSchema* newValues, uint64_t length, ...) {
    va_list arg;
    va_start(arg, length);
    query* q = createQuery(schemaName, cond, INSERT_QUERY, newValues, length, arg);
    va_end(arg);
    return q;
}

query* selectOrDeleteQuery(const char* schemaName, condition* cond, uint64_t length, ...) {
    if (schemaName && strlen(schemaName) <= 12) {
        va_list arg;
        va_start(arg, length);
        query* q = createQuery(schemaName, cond, SELECT_QUERY, NULL, length, arg);
        va_end(arg);
        return q;
    }
    return NULL;
}

query* updateQuery(const char* schemaName, condition* cond, documentSchema* newValues, uint64_t length, ...) {
    if (schemaName && strlen(schemaName) <= 12) {
        va_list arg;
        va_start(arg, length);
        query* q = createQuery(schemaName, cond, UPDATE_QUERY, newValues, length, arg);
        va_end(arg);
        return q;
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

bool checkRoot(zgdbFile* file, documentSchema* schema) {
    zgdbIndex index = getIndex(file, file->header.indexOfRoot);
    if (index.flag == INDEX_ALIVE) {
        documentHeader header;
        fseeko64(file->f, index.offset, SEEK_SET); // спуск в корень по смещению
        // Читаем заголовок корня и проверяем, соответствует ли имя корня в запросе его истинному имени:
        if (fread(&header, sizeof(documentHeader), 1, file->f) && !strcmp(schema->name, header.schemaName)) {
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
    if (q && q->type == INSERT_QUERY) {
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
        return find(file, NULL, file->header.indexOfRoot, q, &removeDocument);
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