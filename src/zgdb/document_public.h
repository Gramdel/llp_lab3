#ifndef _DOCUMENT_PUBLIC_H_
#define _DOCUMENT_PUBLIC_H_

/* Максимальное возможное число индексов - (2^40-1), т.е. максимальный номер индекса - (2^40-2). Тогда можно использовать
 * число (2^40-1) в качестве специального номера индекса, обозначающего, что документ не существует: */
#define DOCUMENT_NOT_EXIST 0xFFFFFFFFFF

#include <stdint.h>
#include <stdbool.h>

#include "format_public.h"
#include "schema_public.h"
#include "element_public.h"

/* Структура для загруженного в память документа */
typedef struct document document;

// TODO: описание
void destroyDocument(document* doc);

/* Функция для вывода документа. */
void printDocument(zgdbFile* file, document* doc);

// TODO: описание
element* getElementFromDocument(document* doc, const char* key);

// TODO: описание
documentSchema* getSchemaFromDocument(document* doc);

#endif