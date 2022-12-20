#ifndef _DOCUMENT_H_
#define _DOCUMENT_H_

#define DOCUMENT_BUF_SIZE 500000000 // при перемещении большие документы будут перемещаться кусками по 500Мб

#include "format_public.h"
#include "document_public.h"

struct __attribute__((packed)) documentId {
    uint32_t timestamp; // время создания документа в секундах с эпохи UNIX
    int64_t offset; // смещение документа относительно начала файла на момент создания документа
};

/* Структура для заголовка документа. */
typedef struct __attribute__((packed)) documentHeader {
    uint64_t size : 40; // (5 байт) размер документа в байтах
    uint64_t indexNumber : 40; // (5 байт) номер индекса, прикрепленного к документу
    uint64_t parentIndexNumber : 40; // (5 байт) номер индекса, прикрепленного к родительскому документу
    documentId id; // id, привязанный к документу
} documentHeader;

struct str {
    uint32_t size; // размер строки с учётом терминатора
    char* data;
};

struct element {
    uint8_t type; // тип элемента
    char key[13]; // ключ элемента
    union {
        int32_t integerValue;
        double doubleValue;
        uint8_t booleanValue;
        str stringValue; // строка
        uint64_t documentValue : 40; // (5 байт) номер индекса, прикрепленного ко вложенному документу
    };
};

/* Структура для схемы данных. */
struct documentSchema{
    element* elements;
    uint64_t elementCount;
    uint64_t capacity;
};

#endif