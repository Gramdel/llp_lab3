#ifndef _QUERY_PUBLIC_H_
#define _QUERY_PUBLIC_H_

#include "element_public.h"

/* Структура для условия. В случае унарной операции (она одна - OP_NOT), второй операнд (condition2) равен
 * первому (condition1) */
typedef struct condition condition;

condition* condEqual(element* el);
condition* condNotEqual(element* el);
condition* condLess(element* el);
condition* condLessOrEqual(element* el);
condition* condGreater(element* el);
condition* condGreaterOrEqual(element* el);
condition* condAnd(condition* cond1, condition* cond2);
condition* condOr(condition* cond1, condition* cond2);
condition* condNot(condition* cond);

#endif