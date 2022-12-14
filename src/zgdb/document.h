#ifndef _DOCUMENT_H_
#define _DOCUMENT_H_

#define DOCUMENT_HAS_NO_PARENT 0xFFFFFFFFFF // максимальный номер индекса - (2^40-1), поэтому 2^40 можно использовать как флаг
#define DOCUMENT_BUF_SIZE 500000000 // при перемещении большие документы будут перемещаться кусками по 500мб

#include <stdbool.h>
#include "format.h"

/* Идентификаторы для типов данных в документе */
typedef enum {
    TYPE_NOT_EXIST = 0, // тип, сигнализирующий об ошибке
    TYPE_INT = 1, // для int32_t
    TYPE_DOUBLE = 2, // для double
    TYPE_BOOLEAN = 3, // для boolean (uint8_t)
    TYPE_STRING = 4, // для строки
    TYPE_EMBEDDED_DOCUMENT = 5 // для вложенного документа
} elementType;

/* Структура для строки */
typedef struct {
    uint32_t size;
    unsigned char* data;
} str;

/* Структура для элемента документа */
typedef struct {
    uint8_t type; // тип элемента
    char key[13]; // ключ элемента
    union {
        int32_t integerValue;
        double doubleValue;
        uint8_t booleanValue;
        str* stringValue; // указатель на строку
        uint64_t documentValue : 40; // (5 байт) номер индекса, прикрепленного ко вложенному документу
    };
} element;

/* Структура для id, привязанного к документу. Для вложенных - id нулевой */
typedef struct __attribute__((packed)) {
    uint32_t timestamp; // время создания документа в секундах с эпохи UNIX
    int64_t offset; // смещение документа относительно начала файла на момент создания документа
} documentId;

/* Структура для заголовка документа */
typedef struct __attribute__((packed)) {
    uint64_t size : 40; // (5 байт) размер документа в байтах
    uint64_t indexNumber : 40; // (5 байт) номер индекса, прикрепленного к документу
    uint64_t parentIndexNumber : 40; // (5 байт) номер индекса, прикрепленного к родительскому документу
    documentId id; // id, привязанный к документу
} documentHeader;

// TODO: не используется! Надо что-то сделать...
/* Структура для документа (документа) */
typedef struct document {
    documentHeader* header;
    element* elements;
    size_t elementCount;
} document;

/* Структура для схемы данных */
typedef struct {
    element* elements;
    size_t elementCount;
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

/* Функция для удаления документа из файла. Возвращает false при неудаче */
bool removeDocument(zgdbFile* file, sortedList* list, uint64_t i);

/* Функция для чтения элемента из документа по ключу. При неудаче возвращает элемент с типом TYPE_NOT_EXIST */
element readElement(zgdbFile* file, const char* neededKey, uint64_t i);

#endif