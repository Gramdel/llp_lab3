#include <string.h>
#include <malloc.h>
#include <stdarg.h>

#include "query.h"
#include "condition.h"
#include "element.h"
#include "iterator.h"
#include "schema.h"

query* createQuery(documentSchema* schema, condition* cond, bool isUpdating, documentSchema* newValues, uint64_t length,
                   va_list arg) {
    if (schema && checkNewValues(schema, newValues)) {
        // TODO: get rid
    }
    query* q = malloc(sizeof(query));
    if (q) {
        q->schema = schema;
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
    return NULL;
}

query* insertQuery(documentSchema* schema, condition* cond, documentSchema* newValues, uint64_t length, ...) {
    va_list arg;
    va_start(arg, length);
    query* q = createQuery(schema, cond, true, newValues, length, arg);
    va_end(arg);
    return q;
}

query* selectOrDeleteQuery(documentSchema* schema, condition* cond, uint64_t length, ...) {
    va_list arg;
    va_start(arg, length);
    query* q = createQuery(schema, cond, false, NULL, length, arg);
    va_end(arg);
    return q;
}

query* updateQuery(documentSchema* schema, condition* cond, documentSchema* newValues, uint64_t length, ...) {
    va_list arg;
    va_start(arg, length);
    query* q = createQuery(schema, cond, true, newValues, length, arg);
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

bool checkNewValues(documentSchema* schema, documentSchema* newValues) {
    // Если newValues == NULL, то сразу возвращаем true:
    if (!newValues) {
        return true;
    }
    /* Делаем проверки на то, что:
     * 1. schema и newValues - разные указатели;
     * 2. Их имена соответствуют;
     * 3. В newValues вообще есть элементы. */
    if (schema != newValues && !strcmp(schema->name, newValues->name) && newValues->length) {
        for (uint64_t i = 0; i < newValues->length; i++) {
            element* tmp = getElementFromSchema(schema, newValues->elements[i]->key);
            if (!tmp || tmp->type != newValues->elements[i]->type) {
                return false;
            }
        }
        return true;
    }
    return false;
}

iterator* executeSelect(zgdbFile* file, query* q) {
    if (file->header.indexOfRoot != DOCUMENT_NOT_EXIST) {
        iterator* it = createIterator();
        if (it) {
            if (q) {
                if (checkRoot(file, q->schema) && !q->isUpdating &&
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
    if (q && file->header.indexOfRoot != DOCUMENT_NOT_EXIST && checkRoot(file, q->schema) && !q->isUpdating) {
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
    if (q && file->header.indexOfRoot != DOCUMENT_NOT_EXIST && checkRoot(file, q->schema) && q->isUpdating) {
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

bool executeInsert(zgdbFile* file, query* q) {
    if (q && q->isUpdating) {
        return find(file, NULL, file->header.indexOfRoot, q, &insertDocument);
    }
    return false;
}

bool executeNestedQuery(zgdbFile* file, query* q, uint64_t indexNumber,
                        bool (* mutate)(zgdbFile*, uint64_t, documentSchema*), iterator* it) {
    bool match = checkDocument(file, indexNumber, q->schema, q->cond);
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
                                int64_t offsetOfNextElement = ftello64(file->f) - index.offset;
                                if (!executeNestedQuery(file, q->nestedQueries[i], el.documentValue.indexNumber, mutate,
                                                        it)) {
                                    destroyIterator(it);
                                    return NULL;
                                }
                                /* executeNestedQuery может вызвать update, и, если этот документ обновляется или
                                 * перемещается, то следующий элемент будет находиться уже не там, где находится
                                 * указатель в файле. Нужно переместить указатель: */
                                index = getIndex(file, parentIndexNumber);
                                fseeko64(file->f, index.offset + offsetOfNextElement, SEEK_SET);
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

// TODO: подумать, как добавлять детей, если lastChildIndexNumber отсутствует. (то есть например документ пустой, и мы создаём корень и его детей)
bool find(zgdbFile* file, iterator* it, uint64_t indexNumber, query* q, bool (* mutate)(zgdbFile*, uint64_t*, query*)) {
    bool match = checkDocument(file, indexNumber, q->schema, q->cond);
    resetCondition(q->cond); // сброс cond.met
    // TODO: нужен чек в removeDocument на то, что есть nestedQueries (вроде так).
    if (match && (!mutate || mutate(file, &indexNumber, q))) {
        // Если выше было вызвано рекурсивное добавление новых документов, то всё уже добавлено. Можно выходить:
        if (!q->schema) {
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
                            if (fread(&header, sizeof(documentHeader), 1, file->f)) {
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
            return addRef(it, (documentRef) { indexNumber });
        }
        return true;
    }
    return false;
}