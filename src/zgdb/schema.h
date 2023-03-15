#ifndef _SCHEMA_H_
#define _SCHEMA_H_

#include "schema_public.h"
#include "element_public.h"

struct documentSchema {
    element* elements; // указатель на массив элементов
    uint64_t elementCount; // текущее количество элементов в массиве
    uint64_t capacity; // размер массива (максимальное число элементов, которое он может вместить без реаллокаций)
    char name[13];
};

/* Функция для расчёта размера будущего документа по схеме. */
uint64_t calcDocumentSize(documentSchema* schema);

#endif