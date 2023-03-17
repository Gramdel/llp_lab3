#ifndef _QUERY_PUBLIC_H_
#define _QUERY_PUBLIC_H_

#include "condition_public.h"
#include "iterator_public.h"

typedef struct query query;

iterator* executeSelect(zgdbFile* file, query* q);
iterator* executeDelete(zgdbFile* file, query* q);
iterator* executeUpdate(zgdbFile* file, query* q);
query* selectOrDeleteQuery(documentSchema* schema, condition* cond, uint64_t length, ...);
query* updateQuery(documentSchema* schema, condition* cond, documentSchema* newValues, uint64_t length, ...);

#endif