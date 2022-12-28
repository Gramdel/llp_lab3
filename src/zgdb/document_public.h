#ifndef _DOCUMENT_PUBLIC_H_
#define _DOCUMENT_PUBLIC_H_

#define DOCUMENT_NOT_EXIST 0xFFFFFFFFFF // максимальный номер индекса - (2^40-1), поэтому 2^40 можно использовать как флаг

#include <stdint.h>
#include <stdbool.h>

#include "format_public.h"

/* Структура для "ссылки" на документ.
 * Фактически, обёртка для номера индекса документа в файле.*/
typedef struct documentRef documentRef;

/* Структура для схемы данных. */
typedef struct documentSchema documentSchema;

/* Функция для добавления нового документа в файл. Если у документа есть "дети", то создаёт их, спускается в их заголовки
 * и записывает в них информацию об индексе добавляемого документа (родителя).
 * Возвращает ссылку на документ или NULL (при неудаче). */
documentRef* writeDocument(zgdbFile* file, documentSchema* schema);

/* Функция для удаления документа из файла. Вне зависимости от результата, делает ссылку на документ недоступной.
 * Возвращает false при неудаче. */
bool removeDocument(zgdbFile* file, documentRef* ref);

/* Функция для вывода документа. */
void printDocument(zgdbFile* file, documentRef* ref);

/* Функция для получения ссылки на документ с определённым ID. Принимает ID в виде строки из 24 символов.
 * ВНИМАНИЕ: Если ID получен не после вывода, а напрямую (через HEX редактор), то нужно его сначала перевести в Big Endian.
 * Возвращает NULL при неудаче. */
documentRef* getDocumentByID(zgdbFile* file, char* idAsString);

/* Функция для уничтожения ссылки на документ. С документом в файле ничего не делает! */
void destroyDocumentRef(documentRef* ref);

/* Функция для создания схемы с изначальной вместимостью, равной аргументу.
 * Возвращает NULL при неудаче. */
documentSchema* createSchema(uint64_t capacity);

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