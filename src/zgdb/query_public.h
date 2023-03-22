#ifndef _QUERY_PUBLIC_H_
#define _QUERY_PUBLIC_H_

#include "condition_public.h"
#include "iterator_public.h"

typedef struct query query;


query* createSelectQuery(const char* schemaName, condition* cond);

query* createInsertQuery(const char* schemaName, documentSchema* newValues, condition* cond);

query* createUpdateQuery(const char* schemaName, documentSchema* newValues, condition* cond);

query* createDeleteQuery(const char* schemaName, condition* cond);

// TODO: описание всего и вся!
void destroyQuery(query* q);

bool addNestedQuery(query* q, query* nq);

bool executeSelect(zgdbFile* file, bool* error, iterator** it, query* q);

bool executeInsert(zgdbFile* file, bool* error, query* q);

bool executeUpdate(zgdbFile* file, bool* error, query* q);

bool executeDelete(zgdbFile* file, bool* error, query* q);

#endif