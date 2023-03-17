#ifndef _CONDITION_PUBLIC_H_
#define _CONDITION_PUBLIC_H_

#include "element_public.h"

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