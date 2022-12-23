#ifndef _DOCUMENT_H_
#define _DOCUMENT_H_

#include <stdint.h>

#include "format.h"
#include "document_public.h"
#include "element_public.h"

struct __attribute__((packed)) documentId {
    uint32_t timestamp; // время создания документа в секундах с эпохи UNIX
    int64_t offset; // смещение документа относительно начала файла на момент создания документа
};

struct documentRef {
    uint64_t indexNumber : 40; // (5 байт) номер индекса, прикрепленного к документу
};

struct documentSchema{
    element* elements;
    uint64_t elementCount;
    uint64_t capacity;
};

/* Структура для заголовка документа. */
typedef struct __attribute__((packed)) documentHeader {
    uint64_t size : 40; // (5 байт) размер документа в байтах
    uint64_t indexNumber : 40; // (5 байт) номер индекса, прикрепленного к документу
    uint64_t parentIndexNumber : 40; // (5 байт) номер индекса, прикрепленного к родительскому документу
    documentId id; // id, привязанный к документу
} documentHeader;

/* Функция для перемещения документов, идущих в файле сразу после индексов, в новое место (в конец файла или дырку).
 * Продлевает массив индексов, используя освобождённое место. Если освободившееся место не делится нацело на размер
 * индекса, то остаток сохраняется в заголовке файла в firstDocumentOffset.
 * Возвращает false при неудаче. */
bool moveFirstDocuments(zgdbFile* file);

/* Функция для рекурсивного удаления вложенных документов.
 * Продлевает массив индексов, используя освобождённое место. Если освободившееся место не делится нацело на размер
 * индекса, то остаток сохраняется в заголовке файла в firstDocumentOffset.
 * Возвращает статус индекса, привязанного к удаляемому документу, или INDEX_NOT_EXIST (при неудаче). */
indexFlag removeEmbeddedDocument(zgdbFile* file, uint64_t childIndexNumber, uint64_t parentIndexNumber);

#endif