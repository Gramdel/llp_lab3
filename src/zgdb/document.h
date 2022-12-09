#ifndef _DOCUMENT_H_
#define _DOCUMENT_H_

#include <stdbool.h>
#include "format.h"

/* Идентификаторы для типов данных в документе */
typedef enum {
    TYPE_INT = 0x01, // для int32_t
    TYPE_DOUBLE = 0x02, // для double
    TYPE_BOOLEAN = 0x03, // для boolean (uint8_t)
    TYPE_STRING = 0x04, // для строки
    TYPE_EMBEDDED_DOCUMENT = 0x05 // для вложенного документа
} elementType;

/* Терминаторы в документе */
typedef enum {
    NULL_TERMINATOR = 0x00, // терминатор для строк и ключей в документе
    DOCUMENT_TERMINATOR = 0xFF, // терминатор для определения границ документов
    EMBEDDED_DOCUMENT_TERMINATOR = 0xFE // терминатор для вложенного документа
} terminator;

/* Структура для строки */
typedef struct {
    uint32_t size;
    unsigned char* data;
} str;

typedef struct document document;

/* Структура для элемента документа */
typedef struct {
    uint8_t type; // тип элемента
    char key[13]; // ключ элемента
    union {
        int32_t integerValue;
        double doubleValue;
        uint8_t booleanValue;
        str* stringValue; // указатель на строку
        uint64_t documentValue : 40; // порядковый номер индекса, прикрепленного ко вложенному документу
    };
} element;

/* Структура для id, привязанного к документу. Для вложенных - id нулевой */
typedef struct __attribute__((packed)) {
    uint32_t timestamp; // время создания документа в секундах с эпохи UNIX
    uint64_t offset; // смещение документа относительно начала файла на момент создания документа
} documentId;

/* Структура для заголовка документа */
typedef struct __attribute__((packed)) {
    uint64_t size : 40; // (5 байт) размер документа в байтах
    uint64_t indexOrder : 40; // (5 байт) порядковый номер индекса, прикрепленного к документу
    uint64_t parentIndexOrder : 40; // (5 байт) порядковый номер индекса, прикрепленного к родительскому документу
    // TODO: какого хрена размер 16?
    documentId id; // id, привязанный к документу
} documentHeader;

/* Структура для документа (документа) */
typedef struct document {
    documentHeader* header;
    element* elements;
    size_t elementNumber;
} document;

/* Структура для схемы данных */
typedef struct {
    element* elements;
    size_t elementNumber;
    size_t capacity;
} documentSchema;

/* Функции для добавления данных в схему. Возвращают false при неудаче */
bool addIntegerToSchema(documentSchema* schema, const char* key, int32_t value);

bool addDoubleToSchema(documentSchema* schema, const char* key, double value);

bool addBooleanToSchema(documentSchema* schema, const char* key, uint8_t value);

bool addStringToSchema(documentSchema* schema, const char* key, str* value);

bool addDocumentToSchema(documentSchema* schema, const char* key, uint64_t value);

/* Функция для инициализации схемы с определенным начальным размером. */
documentSchema* createSchema(size_t capacity);

/* Функция для уничтожения схемы */
void destroySchema(documentSchema* schema);

/* Функция для добавления нового документа в файл. Возвращает false при неудаче */
bool writeDocument(zgdbFile* file, sortedList* list, documentSchema* schema);

/* Функция для чтения элемента из документа по ключу. Возвращает null при неудаче */
element* readElementFromDocument(zgdbFile* file, const char* neededKey, uint64_t i);

#endif