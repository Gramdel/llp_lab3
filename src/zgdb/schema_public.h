#ifndef _SCHEMA_PUBLIC_H_
#define _SCHEMA_PUBLIC_H_

#include <stdint.h>
#include <stdbool.h>

/* Структура для схемы данных. */
typedef struct documentSchema documentSchema;

/* Функция для создания схемы с изначальной вместимостью capacity и именем name.
 * Имя указывать обязательно, а вот в capacity можно передать 0 (если заранее неизвестно, сколько нужно).
 * Возвращает NULL при неудаче. */
documentSchema* createSchema(const char* name, uint64_t capacity);

/* Функция для уничтожения схемы. */
void destroySchema(documentSchema* schema);

/* Функция для добавления в схему элемента типа TYPE_INT.
 * Возвращает false при неудаче. */
bool addIntegerToSchema(documentSchema* schema, char* key, int32_t value);

/* Функция для добавления в схему элемента типа TYPE_DOUBLE.
 * Возвращает false при неудаче. */
bool addDoubleToSchema(documentSchema* schema, char* key, double value);

/* Функция для добавления в схему элемента типа TYPE_BOOLEAN.
 * Возвращает false при неудаче. */
bool addBooleanToSchema(documentSchema* schema, char* key, uint8_t value);

/* Функция для добавления в схему элемента типа TYPE_STRING.
 * Возвращает false при неудаче. */
bool addStringToSchema(documentSchema* schema, char* key, char* value);

/* Функция для добавления в схему элемента типа TYPE_EMBEDDED_DOCUMENT с определённой схемой. */
bool addEmbeddedDocumentToSchema(documentSchema* schema, char* key, documentSchema* embeddedSchema);

#endif