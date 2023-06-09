#ifndef _ELEMENT_PUBLIC_H_
#define _ELEMENT_PUBLIC_H_

#include <stdint.h>
#include <stdbool.h>
#include <glib.h>

/* Структура для элемента схемы документа. */
typedef struct element element;

/* Функция для создания элемента типа int. Ключ обязателен!
 * Возвращает NULL при неудаче. */
element* intElement(const char* key, int32_t value);

/* Функция для создания элемента типа double. Ключ обязателен!
 * Возвращает NULL при неудаче. */
element* doubleElement(const char* key, double value);

/* Функция для создания элемента типа bool. Ключ обязателен!
 * Возвращает NULL при неудаче. */
element* booleanElement(const char* key, bool value);

/* Функция для создания элемента типа string. Ключ обязателен; значение не должно быть NULL!
 * Возвращает NULL при неудаче. */
element* stringElement(const char* key, char* value);

/* Функция для создания элемента без value. Ключ обязателен!
 * Возвращает NULL при неудаче. */
element* noValueElement(const char* key);

/* Функция для уничтожения элемента. */
void destroyElement(element* el);

/* Функция для вывода элемента на экран. */
GString* printElement(element *el);

#endif