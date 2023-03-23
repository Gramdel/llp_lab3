#ifndef _ELEMENT_PUBLIC_H_
#define _ELEMENT_PUBLIC_H_

#include <stdint.h>
#include <stdbool.h>

#include "format_public.h"

/* Структура для строки. В конце строки обязательно должен быть терминатор! */
typedef struct str str;

/* Структура для элемента документа. */
typedef struct element element;

element* intElement(const char* key, int32_t value);
element* doubleElement(const char* key, double value);
element* booleanElement(const char* key, bool value);
element* stringElement(const char* key, char* value);

/* Функция для уничтожения элемента. */
void destroyElement(element* el);

/* Функция для вывода элемента на экран. */
void printElement(element *el);

#endif