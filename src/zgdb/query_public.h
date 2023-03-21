#ifndef _QUERY_PUBLIC_H_
#define _QUERY_PUBLIC_H_

#include "condition_public.h"
#include "iterator_public.h"

typedef struct query query;

query* selectOrDeleteQuery(const char* schemaName, condition* cond, uint64_t length, ...);
query* insertQuery(const char* schemaName, condition* cond, documentSchema* newValues, uint64_t length, ...);
query* updateQuery(const char* schemaName, condition* cond, documentSchema* newValues, uint64_t length, ...);

// TODO: описание всего и вся!
void destroyQuery(query* q);

iterator* executeSelect(zgdbFile* file, query* q);
bool executeInsert(zgdbFile* file, query* q);
bool executeUpdate(zgdbFile* file, query* q);
bool executeDelete(zgdbFile* file, query* q);

#endif