#ifndef _INDEX_H_
#define _INDEX_H_

#include <stdbool.h>
#include "format.h"

/* Флаги для индексов */
typedef enum {
    INDEX_NEW = 0, // индекс только что создан и ещё не привязан к блоку
    INDEX_ALIVE = 1, // индекс привязан к использующемуся ("живому") блоку
    INDEX_DEAD = 2 // индекс привязан к неиспользующемуся ("мертвому") блоку
} indexFlag;

/* Структура для индекса в ZGDB файле */
typedef struct __attribute__((packed)) {
    uint8_t flags; // флаги (т.е. мета-информация об индексе)
    uint64_t offset; // смещение блока относительно начала файла
} zgdbIndex;

/* Функция для получения индекса по его порядковому номеру. Возвращает null при неудаче */
zgdbIndex* getIndex(zgdbFile* file, uint64_t i);

/* Функция, помечающая индекс как INDEX_DEAD по его порядковому номеру. Возвращает false при неудаче */
bool killIndex(zgdbFile* file, uint64_t i);

/* Функция, меняющая offset в INDEX_ALIVE индексе. При неудаче возвращает false */
bool changeOffset(zgdbFile* file, uint64_t order, uint64_t offset);

#endif