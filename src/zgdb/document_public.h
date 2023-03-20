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

/* Структура для "ссылки" на документ.
 * Фактически, обёртка для номера индекса документа в файле.*/
typedef struct documentRef documentRef;

/* Структура для загруженного в память документа */
typedef struct document document;

/* Функция для добавления нового документа в файл. Если у документа есть "дети", то создаёт их, спускается в их заголовки
 * и записывает в них информацию об индексе добавляемого документа (родителя).
 * Возвращает ссылку на документ или NULL (при неудаче). TODO*/
documentRef* writeDocument(zgdbFile* file, documentSchema* schema, uint64_t brotherIndexNumber);

document* readDocument(zgdbFile* file, uint64_t indexNumber);

/* Функция для удаления документа из файла. Вне зависимости от результата, делает ссылку на документ недоступной.
 * Возвращает false при неудаче. */
bool removeDocument(zgdbFile* file, documentRef* ref);

/* Функция для вывода документа. */
void printDocument(zgdbFile* file, document* doc);

/* Функция для уничтожения ссылки на документ. С документом в файле ничего не делает! */
void destroyDocumentRef(documentRef* ref);

bool createRoot(zgdbFile* file, documentSchema* schema);

element* getElementFromDocument(document* doc, const char* key);

documentSchema* getSchemaFromDocument(document* doc);

#endif