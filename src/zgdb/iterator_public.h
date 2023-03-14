#ifndef _ITERATOR_PUBLIC_H_
#define _ITERATOR_PUBLIC_H_

#include "format_public.h"

typedef struct iterator iterator;

bool hasNext(iterator* it);

documentSchema* next(zgdbFile* file, iterator* it);

void destroyIterator(iterator* it);

#endif