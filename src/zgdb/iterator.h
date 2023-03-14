#ifndef _ITERATOR_H_
#define _ITERATOR_H_

#include "document_public.h"
#include "format_public.h"

typedef struct iterator iterator;
struct iterator {

};

bool hasNext(iterator* it);
documentSchema* next(zgdbFile* file, iterator* it);

#endif