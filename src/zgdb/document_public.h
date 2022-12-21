#ifndef _DOCUMENT_PUBLIC_H_
#define _DOCUMENT_PUBLIC_H_

#define DOCUMENT_NOT_EXIST 0xFFFFFFFFFF // максимальный номер индекса - (2^40-1), поэтому 2^40 можно использовать как флаг

#include <stdint.h>
#include <stdbool.h>

#include "format_public.h"
#include "schema_public.h"

/* Структура для id, привязанного к документу. */
typedef struct documentId documentId;

/* Функция для добавления нового документа в файл. Если у документа есть "дети", то спускается в их заголовки и
 * записывает в них информацию об индексе добавляемого документа (родителя).
 * ВНИМАНИЕ: Если к моменту вызова этой функции дети не были записаны в файл, вернёт DOCUMENT_NOT_EXIST.
 * ВНИМАНИЕ: Если у ребёнка уже есть родитель, вернёт DOCUMENT_NOT_EXIST.
 * Возвращает номер присвоенного индекса или DOCUMENT_NOT_EXIST (в случае ошибки). */
uint64_t writeDocument(zgdbFile* file, documentSchema* schema);

/* Функция для удаления документа из файла.
 * Возвращает false при неудаче. */
bool removeDocument(zgdbFile* file, uint64_t i);

#endif