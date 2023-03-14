#ifndef _DOCUMENT_H_
#define _DOCUMENT_H_

#include <stdint.h>

#include "format.h"
#include "query.h"
#include "document_public.h"
#include "element_public.h"

/* Структура для id, привязанного к документу. */
typedef struct __attribute__((packed)) documentId {
    uint32_t timestamp; // время создания документа в секундах с эпохи UNIX
    int64_t offset; // смещение документа относительно начала файла на момент создания документа
} documentId;

struct documentRef {
    uint64_t indexNumber : 40; // (5 байт) номер индекса, прикрепленного к документу
};

struct documentSchema {
    element* elements; // указатель на массив элементов
    uint64_t elementCount; // текущее количество элементов в массиве
    uint64_t capacity; // размер массива (максимальное число элементов, которое он может вместить без реаллокаций)
    char name[13];
};

/* Структура для заголовка документа. */
typedef struct __attribute__((packed)) documentHeader {
    uint64_t size : 40; // (5 байт) размер документа в байтах
    uint64_t indexNumber : 40; // (5 байт) номер индекса, прикрепленного к документу
    uint64_t parentIndexNumber : 40; // (5 байт) номер индекса, прикрепленного к родительскому документу
    documentId id; // id, привязанный к документу
    char schemaName[13]; // имя схемы, по которой построен документ
} documentHeader;

/* Функция для расчёта размера будущего документа по схеме. */
uint64_t calcDocumentSize(documentSchema* schema);

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

documentRef* findAllDocuments(zgdbFile* file, documentRef* parent, documentSchema* neededSchema, condition* cond);

#endif