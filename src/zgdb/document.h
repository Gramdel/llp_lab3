#ifndef _DOCUMENT_H_
#define _DOCUMENT_H_

#define DOCUMENT_HAS_NO_PARENT 0xFFFFFFFFFF // максимальный номер индекса - (2^40-1), поэтому 2^40 можно использовать как флаг
#define DOCUMENT_BUF_SIZE 500000000 // при перемещении большие документы будут перемещаться кусками по 500мб
#define DOCUMENT_START_MARK 0xFF

#include <stdint.h>
#include <stdbool.h>

#include "format_public.h"

/* Идентификаторы для типов данных в документе */
typedef enum {
    TYPE_NOT_EXIST = 0, // тип, сигнализирующий об ошибке
    TYPE_INT = 1, // для int32_t
    TYPE_DOUBLE = 2, // для double
    TYPE_BOOLEAN = 3, // для boolean (uint8_t)
    TYPE_STRING = 4, // для строки
    TYPE_EMBEDDED_DOCUMENT = 5 // для вложенного документа
} elementType;

/* Структура для строки. В конце строки обязательно должен быть терминатор! */
typedef struct {
    uint32_t size; // размер строки с учётом терминатора
    char* data;
} str;

/* Структура для элемента документа */
typedef struct {
    uint8_t type; // тип элемента
    char key[13]; // ключ элемента
    union {
        int32_t integerValue;
        double doubleValue;
        uint8_t booleanValue;
        str stringValue; // строка
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
    uint8_t mark; // должна быть записана метка, означающая начало блока
    uint64_t size : 40; // (5 байт) размер документа в байтах
    uint64_t indexNumber : 40; // (5 байт) номер индекса, прикрепленного к документу
    uint64_t parentIndexNumber : 40; // (5 байт) номер индекса, прикрепленного к родительскому документу
    documentId id; // id, привязанный к документу
} documentHeader;

/* Структура для схемы данных */
typedef struct {
    element* elements;
    uint64_t elementCount;
    uint64_t capacity;
} documentSchema;

/* Функции для добавления данных в схему. Возвращают false при неудаче */
bool addIntegerToSchema(documentSchema* schema, char* key, int32_t value);

bool addDoubleToSchema(documentSchema* schema, char* key, double value);

bool addBooleanToSchema(documentSchema* schema, char* key, uint8_t value);

bool addStringToSchema(documentSchema* schema, char* key, char* value);

bool addDocumentToSchema(documentSchema* schema, char* key, uint64_t value);

/* Функция для инициализации схемы с определенным начальным размером. */
documentSchema* createSchema(uint64_t capacity);

/* Функция для уничтожения схемы */
void destroySchema(documentSchema* schema);

/* Функция для добавления нового документа в файл. Возвращает false при неудаче */
bool writeDocument(zgdbFile* file, documentSchema* schema);

/* Функция для удаления документа из файла. Возвращает false при неудаче */
bool removeDocument(zgdbFile* file, uint64_t i);

/* Функция для чтения элемента из документа по ключу. При неудаче возвращает элемент с типом TYPE_NOT_EXIST.
 * ВНИМАНИЕ: если элемент - строка, то обязателен вызов destroyStringElement, чтобы очистить память */
element readElement(zgdbFile* file, char* neededKey, uint64_t i);

/* Функция для очистки памяти, которая была аллоцирована при получении строки из файла.
 * ВНИМАНИЕ: вызывать только для элементов, полученных с помощью readElement. Иначе функция очистит строковый литерал! */
void destroyStringElement(element el);

#endif