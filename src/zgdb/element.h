#ifndef _ELEMENT_H_
#define _ELEMENT_H_

#include "element_public.h"

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

/* Функция для записи элемента в файл.
 * ВНИМАНИЕ: Предполагается, что к моменту вызова функции fseek уже сделан.
 * Возвращает количество записанных байт. */
uint64_t writeElement(zgdbFile* file, element* el, uint64_t parentIndexNumber);

/* Функция для поиска элемента в документе. Устанавливает с помощью fseek смещение на начало элемента.
 * Возвращает тип найденного элемента или TYPE_NOT_EXIST (при ошибке). */
elementType navigateToElement(zgdbFile* file, char* neededKey, uint64_t i);

#endif