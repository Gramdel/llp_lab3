#ifndef _DOCUMENT_H_
#define _DOCUMENT_H_

#include <stdint.h>

#include "format_public.h"
#include "document_public.h"
#include "element_public.h"
#include "iterator_public.h"
#include "query_public.h"
#include "../utils/optional.h"

/* Структура для id, привязанного к документу. */
typedef struct __attribute__((packed)) documentId {
    uint32_t timestamp; // время создания документа в секундах с эпохи UNIX
    int64_t offset; // смещение документа относительно начала файла на момент создания документа
} documentId;

/* Структура для заголовка документа. */
typedef struct __attribute__((packed)) documentHeader {
    uint64_t size : 40; // (5 байт) размер документа в байтах
    uint64_t indexNumber : 40; // (5 байт) номер индекса, прикрепленного к документу
    uint64_t lastChildIndexNumber : 40; // (5 байт) номер индекса, прикрепленного к последнему добавленному ребёнку
    uint64_t brotherIndexNumber : 40; // (5 байт) номер индекса, прикрепленного к следующему "брату" документа
    documentId id; // id, привязанный к документу
    char schemaName[13]; // имя схемы, по которой построен документ
} documentHeader;

struct document {
    documentHeader header; // заголовок документа
    documentSchema* schema; // указатель на схему документа
};

// TODO: описание
document* createDocument();

/* Функция для перемещения документов, идущих в файле сразу после индексов, в новое место (в конец файла или дырку).
 * Продлевает массив индексов, используя освобождённое место. Если освободившееся место не делится нацело на размер
 * индекса, то остаток сохраняется в заголовке файла в firstDocumentOffset.
 * Возвращает false при неудаче. */
bool moveFirstDocuments(zgdbFile* file);

/* Функция для добавления нового документа в файл. Если у документа есть "дети", то создаёт их, спускается в их заголовки
 * и записывает в них информацию об индексе добавляемого документа (родителя).
 * Возвращает ссылку на документ или NULL (при неудаче). TODO*/
opt_uint64_t writeDocument(zgdbFile* file, documentSchema* schema, uint64_t brotherIndexNumber);

// TODO: описание
document* readDocument(zgdbFile* file, uint64_t indexNumber);

// TODO: описание
bool insertDocument(zgdbFile* file, uint64_t* brotherIndexNumber, query* q);

// TODO: описание
bool updateDocument(zgdbFile* file, uint64_t* indexNumber, query* q);

// TODO: описание
bool removeDocument(zgdbFile* file, documentHeader* parentHeader, uint64_t* indexNumber, query* q);

#endif