#ifndef _QUERY_H_
#define _QUERY_H_

#include <stdbool.h>
#include <stdbool.h>
#include <stdint.h>

#include "query_public.h"

typedef enum queryType {
    SELECT_QUERY,
    INSERT_QUERY,
    UPDATE_QUERY,
    DELETE_QUERY
} queryType;

struct query {
    query** nestedQueries; // массив указателей на вложенные запросы
    uint64_t length; // длина массива вложенных запросов
    const char* schemaName; // имя требуемой схемы
    condition* cond; // условие(я)
    queryType type; // тип запроса
    documentSchema* newValues;  // указатель на схему, в которой хранятся новые значения
};

// TODO: описание
bool find(zgdbFile* file, iterator* it, uint64_t indexNumber, query* q, bool (* mutate)(zgdbFile*, uint64_t*, query*));

#endif