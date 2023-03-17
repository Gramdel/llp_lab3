#ifndef _ELEMENT_H_
#define _ELEMENT_H_

#include "element_public.h"
#include "document.h"

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
        documentRef documentValue; // ссылка на документ; этот тип оказывается в элементе после загрузки из файла
        documentSchema* schemaValue; // указатель на схему; этот тип оказывается в элементе при создании через конструктор
    };
    bool wasLoaded; // служебный флаг, чтобы понять, нужно ли вызывать free(schemaValue) и free(stringValue) при destroy
};

/* Функция для записи элемента в файл.
 * ВНИМАНИЕ: Предполагается, что к моменту вызова функции fseek уже сделан.
 * Возвращает количество записанных байт. */
uint64_t writeElement(zgdbFile* file, element* el, uint64_t parentIndexNumber);

/* Функция для чтения элемента из документа.
 * ВНИМАНИЕ: Предполагается, что к моменту вызова функции fseek уже сделан.
 * Если skipStrings == true, то пропускает (не загружает в память) все строки.
 * Возвращает количество прочитанных байт. */
uint64_t readElement(zgdbFile* file, element* el, bool skipStrings);

bool updateElement(zgdbFile* file, element* newElement, uint64_t parentIndexNumber);

/* Функция для поиска элемента в документе. Устанавливает с помощью fseek смещение на начало элемента.
 * Возвращает тип найденного элемента или TYPE_NOT_EXIST (при ошибке). */
elementType navigateToElement(zgdbFile* file, char* neededKey, uint64_t i);

/* Функция для вывода элементов вложенных документов. При выводе отступ соответствует уровню вложенности (nestingLevel). */
void printElementOfEmbeddedDocument(zgdbFile* file, element* el, uint64_t nestingLevel);

#endif