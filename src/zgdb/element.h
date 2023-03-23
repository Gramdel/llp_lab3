#ifndef _ELEMENT_H_
#define _ELEMENT_H_

#include "format.h"
#include "element_public.h"
#include "document.h"

/* Тип элемента (а точнее, тип значения элемента). */
typedef enum elementType {
    TYPE_NOT_EXIST = 0, // тип для несуществующего элемента
    TYPE_INT = 1, // для int32_t
    TYPE_DOUBLE = 2, // для double
    TYPE_BOOLEAN = 3, // для boolean (uint8_t)
    TYPE_STRING = 4, // для строки
} elementType;

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
    };
    bool wasLoaded; // служебный флаг, чтобы понять, нужно ли вызывать free(stringValue.data) при освобождении памяти
};

// TODO: описание
element* createElement(const char* key, element el);

/* Функция для записи элемента в файл.
 * ВНИМАНИЕ: Предполагается, что к моменту вызова функции fseek уже сделан.
 * Возвращает количество записанных байт. */
uint64_t writeElement(zgdbFile* file, element* el);

/* Функция для чтения элемента из документа.
 * ВНИМАНИЕ: Предполагается, что к моменту вызова функции fseek уже сделан.
 * Если skipStrings == true, то пропускает (не загружает в память) все строки.
 * Возвращает количество прочитанных байт. */
uint64_t readElement(zgdbFile* file, element* el, bool skipStrings);

// TODO: описание
bool updateStringElement(zgdbFile* file, zgdbIndex* index, documentHeader* header, element* oldElement, element* newElement);

#endif