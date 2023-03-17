#ifndef _SCHEMA_H_
#define _SCHEMA_H_

#include "schema_public.h"
#include "element_public.h"

struct documentSchema {
    element** elements; // указатель на массив указателей на элементы
    uint64_t length; // длина массива
    char name[13]; // имя схемы
};

/* Функция для расчёта размера будущего документа по схеме. */
uint64_t calcDocumentSize(documentSchema* schema);

// TODO: описание
bool addElementToSchema(documentSchema* schema, element *el);

#endif