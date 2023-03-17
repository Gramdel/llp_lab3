#ifndef _QUERY_H_
#define _QUERY_H_

#include <stdbool.h>
#include <stdbool.h>
#include <stdint.h>

#include "query_public.h"

struct query {
    query** nestedQueries; // массив указателей на вложенные запросы
    uint64_t length; // длина массива вложенных запросов
    char schemaName[13]; // требуемое имя схемы
    condition* cond; // условие(я)
    bool isUpdating; // переменная-флаг, указывающая на то, нужно ли обновлять значения или нет
    documentSchema* newValues;  // указатель на схему, в которой хранятся новые значения
};

iterator* findAllDocuments(zgdbFile* file, uint64_t parentIndexNumber, query* q, bool (*mutate)(zgdbFile*,uint64_t,documentSchema*));

void destroyQuery(query* q);

#endif