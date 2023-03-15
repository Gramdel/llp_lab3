#ifndef _QUERY_PUBLIC_H_
#define _QUERY_PUBLIC_H_

#include "element_public.h"
#include "iterator_public.h"

#include <stdarg.h>

/* Структура для условия. В случае унарной операции (она одна - OP_NOT), второй операнд (condition2) равен
 * первому (condition1) */
typedef struct condition condition;
typedef struct query query;

condition* condEqual(element* el);
condition* condNotEqual(element* el);
condition* condLess(element* el);
condition* condLessOrEqual(element* el);
condition* condGreater(element* el);
condition* condGreaterOrEqual(element* el);
condition* condAnd(condition* cond1, condition* cond2);
condition* condOr(condition* cond1, condition* cond2);
condition* condNot(condition* cond);

iterator* executeQuery(zgdbFile* file, query* q);
query* createQuery(char* schemaName, condition* cond, uint64_t length, ...);

#endif