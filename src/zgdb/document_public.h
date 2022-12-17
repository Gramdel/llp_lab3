#ifndef _DOCUMENT_PUBLIC_H_
#define _DOCUMENT_PUBLIC_H_

#include <stdint.h>
#include <stdbool.h>

/* Структура для id, привязанного к документу. */
typedef struct documentId documentId;

/* Структура для строки. В конце строки обязательно должен быть терминатор! */
typedef struct str str;

/* Тип элемента (а точнее, тип значения элемента). */
typedef enum elementType {
    TYPE_NOT_EXIST = 0, // тип, сигнализирующий об ошибке
    TYPE_INT = 1, // для int32_t
    TYPE_DOUBLE = 2, // для double
    TYPE_BOOLEAN = 3, // для boolean (uint8_t)
    TYPE_STRING = 4, // для строки
    TYPE_EMBEDDED_DOCUMENT = 5 // для вложенного документа
} elementType;

/* Структура для элемента документа. */
typedef struct element element;

/* Структура для схемы данных. */
typedef struct documentSchema documentSchema;

/* Функция для добавления нового документа в файл.
 * Возвращает false при неудаче. */
bool writeDocument(zgdbFile* file, documentSchema* schema);

/* Функция для удаления документа из файла.
 * Возвращает false при неудаче. */
bool removeDocument(zgdbFile* file, uint64_t i);

/* Функция для инициализации схемы с размером, равным аргументу. */
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

/* Функция для добавления в схему элемента типа TYPE_EMBEDDED_DOCUMENT.
 * Возвращает false при неудаче. */
bool addDocumentToSchema(documentSchema* schema, char* key, uint64_t value);

/* Функция для чтения элемента из документа по ключу.
 * Возвращает элемент с типом TYPE_NOT_EXIST при неудаче.
 * ВНИМАНИЕ: Если тип полученного элемента - TYPE_STRING, то нужно вызвать destroyElement, чтобы очистить память. */
element readElement(zgdbFile* file, char* neededKey, uint64_t i);

/* Функция для уничтожения элемента типа TYPE_STRING.
 * Если тип другой, то функция ничего не делает, поэтому можно спокойно её вызывать после каждого readElement
 * в качестве перестраховки. */
void destroyElement(element el);

/* Функция для вывода элемента на экран. */
void printElement(element el);

/* Функция для получения типа элемента. */
elementType getTypeOfElement(element el);

/* Функция для получения значения из элемента типа TYPE_INT. */
int32_t getIntegerValue(element el);

/* Функция для получения значения из элемента типа TYPE_DOUBLE. */
double getDoubleValue(element el);

/* Функция для получения значения из элемента типа TYPE_BOOLEAN. */
uint8_t getBooleanValue(element el);

/* Функция для получения значения из элемента типа TYPE_STRING. */
str getStringValue(element el);

/* Функция для получения значения из элемента типа TYPE_EMBEDDED_DOCUMENT. */
uint64_t getDocumentValue(element el);

/* Функция для обновления значения у элемента типа TYPE_INT.
 * Возвращает false при неудаче. */
bool updateIntegerValue(documentSchema* schema, char* neededKey, int32_t value, uint64_t i);

/* Функция для обновления значения у элемента типа TYPE_DOUBLE.
 * Возвращает false при неудаче. */
bool updateDoubleValue(documentSchema* schema, char* neededKey, double value, uint64_t i);

/* Функция для обновления значения у элемента типа TYPE_BOOLEAN.
 * Возвращает false при неудаче. */
bool updateBooleanValue(documentSchema* schema, char* neededKey, uint8_t value, uint64_t i);

/* Функция для обновления значения у элемента типа TYPE_STRING.
 * Возвращает false при неудаче. */
bool updateStringValue(documentSchema* schema, char* neededKey, char* value, uint64_t i);

/* Функция для обновления значения у элемента типа TYPE_EMBEDDED_DOCUMENT.
 * Возвращает false при неудаче. */
bool updateDocumentValue(documentSchema* schema, char* neededKey, uint64_t value, uint64_t i);

#endif