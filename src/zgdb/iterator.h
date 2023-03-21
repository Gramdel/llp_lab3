#ifndef _ITERATOR_H_
#define _ITERATOR_H_

#include "iterator_public.h"

#include <malloc.h>

struct iterator {
    uint64_t* refs; // массив номеров индексов документов, которые попали в выборку
    uint64_t length; // длина массива
    int64_t curr; // текущий элемент, на котором находится итератор
};

iterator* createIterator();

bool addRef(iterator* dest, uint64_t ref);

#endif