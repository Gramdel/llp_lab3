#ifndef _DOCUMENT_H_
#define _DOCUMENT_H_

#include <stdint.h>

#include "format.h"
#include "query.h"
#include "document_public.h"
#include "element_public.h"
#include "iterator_public.h"

/* Структура для id, привязанного к документу. */
typedef struct __attribute__((packed)) documentId {
    uint32_t timestamp; // время создания документа в секундах с эпохи UNIX
    int64_t offset; // смещение документа относительно начала файла на момент создания документа
} documentId;

struct documentRef {
    uint64_t indexNumber : 40; // (5 байт) номер индекса, прикрепленного к документу
};

/* Структура для заголовка документа. */
typedef struct __attribute__((packed)) documentHeader {
    uint64_t size : 40; // (5 байт) размер документа в байтах
    uint64_t indexNumber : 40; // (5 байт) номер индекса, прикрепленного к документу
    uint64_t parentIndexNumber : 40; // (5 байт) номер индекса, прикрепленного к родительскому документу
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

// TODO: описание
void destroyDocument(document* doc);

/* Функция для перемещения документов, идущих в файле сразу после индексов, в новое место (в конец файла или дырку).
 * Продлевает массив индексов, используя освобождённое место. Если освободившееся место не делится нацело на размер
 * индекса, то остаток сохраняется в заголовке файла в firstDocumentOffset.
 * Возвращает false при неудаче. */
bool moveFirstDocuments(zgdbFile* file);

/* Функция для рекурсивного удаления вложенных документов.
 * Возвращает статус индекса, привязанного к удаляемому документу, или INDEX_NOT_EXIST (при неудаче). */
indexFlag removeEmbeddedDocument(zgdbFile* file, uint64_t childIndexNumber, uint64_t parentIndexNumber);

/* Функция для рекурсивного вывода вложенных документов. При выводе отступ соответствует уровню вложенности (nestingLevel). */
void printEmbeddedDocument(zgdbFile* file, uint64_t i, uint64_t nestingLevel);

// TODO: описание
bool updateDocument(zgdbFile* file, uint64_t indexNumber, documentSchema* newValues);

// TODO: описание
bool remDocument(zgdbFile* file, uint64_t indexNumber, documentSchema* newValues);

bool insertDocument(zgdbFile* file, uint64_t* brotherIndexNumber, query* q);

#endif