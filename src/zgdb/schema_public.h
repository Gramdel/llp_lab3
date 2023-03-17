#ifndef _SCHEMA_PUBLIC_H_
#define _SCHEMA_PUBLIC_H_

#include <stdint.h>
#include <stdbool.h>

/* Структура для схемы данных. */
typedef struct documentSchema documentSchema;

/* Функция для создания схемы. TODO: дописать
 * Возвращает NULL при неудаче. */
documentSchema* createSchema(const char* name, uint64_t length, ...);

/* Функция для уничтожения схемы. */
void destroySchema(documentSchema* schema);

#endif