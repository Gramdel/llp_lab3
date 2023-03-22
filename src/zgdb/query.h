#ifndef _QUERY_H_
#define _QUERY_H_

#include <stdbool.h>
#include <stdbool.h>
#include <stdint.h>

#include "query_public.h"
#include "document.h"

typedef enum queryType {
    SELECT_QUERY,
    INSERT_QUERY,
    UPDATE_QUERY,
    DELETE_QUERY
} queryType;

struct query {
    queryType type; // тип запроса
    const char* schemaName; // имя требуемой схемы
    documentSchema* newValues;  // указатель на схему, в которой хранятся новые значения
    condition* cond; // условие(я)
    query** nestedQueries; // массив указателей на вложенные запросы
    uint64_t length; // длина массива вложенных запросов
};

query* createQuery(queryType type, const char* schemaName, documentSchema* newValues, condition* cond);

// TODO: описание

bool findAndMutate(zgdbFile* file, bool* error, iterator* it, uint64_t* indexNumber, query* q,
                   bool (* mutate)(zgdbFile*, uint64_t*, query*));

#endif