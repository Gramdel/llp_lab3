#ifndef _ITERATOR_H_
#define _ITERATOR_H_

#include "document_public.h"
#include "iterator_public.h"

#include <malloc.h>

struct iterator {
    documentRef* refs; // массив ссылок на документы
    uint64_t length; // длина массива
    int64_t curr; // текущий элемент, на котором находится итератор
};

iterator* createIterator();

bool addRef(iterator* it, documentRef ref);

bool addAllRefs(iterator* dest, iterator* src);

#endif