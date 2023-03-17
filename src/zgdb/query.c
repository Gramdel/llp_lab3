#include <string.h>
#include <malloc.h>
#include <stdarg.h>

#include "query.h"
#include "condition.h"
#include "element.h"
#include "iterator.h"
#include "schema.h"

query* createQuery(char* schemaName, condition* cond, bool isUpdating, documentSchema* newValues, uint64_t length,
                   va_list arg) {
    if (schemaName && strlen(schemaName) <= 12) {
        query* q = malloc(sizeof(query));
        if (q) {
            strcpy(q->schemaName, schemaName);
            q->cond = cond;
            q->isUpdating = isUpdating;
            q->newValues = newValues;
            if (length) {
                q->nestedQueries = malloc(sizeof(query*) * length);
                if (q->nestedQueries) {
                    for (uint64_t i = 0; i < length; i++) {
                        q->length = i + 1;
                        q->nestedQueries[i] = va_arg(arg, query*);
                        // Если в качестве вложенного query передан NULL или тип q и nq не совпадает, то возвращаем NULL:
                        if (!q->nestedQueries[i] || q->nestedQueries[i]->isUpdating != q->isUpdating) {
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
                if (!q->isUpdating || q->newValues) {
                    return q;
                }
                destroyQuery(q);
            }
        }
    }
    return NULL;
}

query* selectOrDeleteQuery(char* schemaName, condition* cond, uint64_t length, ...) {
    va_list arg;
    va_start(arg, length);
    query* q = createQuery(schemaName, cond, false, NULL, length, arg);
    va_end(arg);
    return q;
}

query* updateQuery(char* schemaName, condition* cond, documentSchema* newValues, uint64_t length, ...) {
    va_list arg;
    va_start(arg, length);
    query* q = createQuery(schemaName, cond, true, newValues, length, arg);
    va_end(arg);
    return q;
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

bool checkRoot(zgdbFile* file, const char* schemaName) {
    zgdbIndex index = getIndex(file, file->header.indexOfRoot);
    if (index.flag == INDEX_ALIVE) {
        documentHeader header;
        fseeko64(file->f, index.offset, SEEK_SET); // спуск в корень по смещению
        // Читаем заголовок корня и проверяем, соответствует ли имя корня в запросе его истинному имени:
        if (fread(&header, sizeof(documentHeader), 1, file->f) && !strcmp(schemaName, header.schemaName)) {
            return true;
        }
    }
    return false;
}

bool executeNestedQuery(zgdbFile* file, query* q, uint64_t indexNumber,
                        bool (* mutate)(zgdbFile*, uint64_t, documentSchema*), iterator* it) {
    bool match = checkDocument(file, indexNumber, q->schemaName, q->cond);
    resetCondition(q->cond); // сброс cond.met
    if (match) {
        // Если мутация вернула false, то это говорит об ошибке, надо возвращать false:
        if (!mutate || mutate(file, indexNumber, q->newValues)) {
            /* Если в q есть вложенные дети, то вызываем поиск по ним, если нет - мы дошли до
             * конца цепочки, добавляем номер документа в итератор: */
            if (q->length) {
                /* Предварительно, если q - обновляющий запрос и для него указаны новые значения,
                 * то его тоже надо занести в итератор, поскольку он затрагивался: */
                if (q->newValues) {
                    return addRef(it, (documentRef) { indexNumber }) &&
                           addAllRefs(it, findAllDocuments(file, indexNumber, q, mutate));
                }
                return addAllRefs(it, findAllDocuments(file, indexNumber, q, mutate));
            }
            return addRef(it, (documentRef) { indexNumber });
        }
        return false;
    }
    return true;
}

iterator* executeSelect(zgdbFile* file, query* q) {
    if (file->header.indexOfRoot != DOCUMENT_NOT_EXIST) {
        iterator* it = createIterator();
        if (it) {
            if (q) {
                if (checkRoot(file, q->schemaName) && !q->isUpdating &&
                    executeNestedQuery(file, q, file->header.indexOfRoot, NULL, it)) {
                    return it;
                }
            } else if (addRef(it, (documentRef) { file->header.indexOfRoot })) {
                return it;
            }
            destroyIterator(it);
        }
    }
    return NULL;
}

iterator* executeDelete(zgdbFile* file, query* q) {
    if (q && file->header.indexOfRoot != DOCUMENT_NOT_EXIST && checkRoot(file, q->schemaName) && !q->isUpdating) {
        iterator* it = createIterator();
        if (it) {
            if (executeNestedQuery(file, q, file->header.indexOfRoot, &remDocument, it)) {
                return it;
            }
            destroyIterator(it);
        }
    }
    return NULL;
}

iterator* executeUpdate(zgdbFile* file, query* q) {
    if (q && file->header.indexOfRoot != DOCUMENT_NOT_EXIST && checkRoot(file, q->schemaName) && q->isUpdating) {
        iterator* it = createIterator();
        if (it) {
            if (executeNestedQuery(file, q, file->header.indexOfRoot, &updateDocument, it)) {
                return it;
            }
            destroyIterator(it);
        }
    }
    return NULL;
}

iterator* findAllDocuments(zgdbFile* file, uint64_t parentIndexNumber, query* q,
                           bool (* mutate)(zgdbFile*, uint64_t, documentSchema*)) {
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
                            // Запускаем обработку вложенного запроса:
                            for (uint64_t i = 0; i < q->length; i++) {
                                if (!executeNestedQuery(file, q->nestedQueries[i], el.documentValue.indexNumber, mutate,
                                                        it)) {
                                    destroyIterator(it);
                                    return NULL;
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