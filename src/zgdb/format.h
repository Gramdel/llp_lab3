#ifndef _FORMAT_H_
#define _FORMAT_H_

#define ZGDB_FILETYPE 0x4244475A
#define ZGDB_DEFAULT_INDEX_CAPACITY 10

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "list.h"
#include "../utils/optional.h"

/* Структура для заголовка ZGDB файла */
typedef struct __attribute__((packed)) {
    uint32_t fileType; // должны быть записаны 4 буквы в UTF-8: ZGDB
    uint8_t firstDocumentOffset; // смещение первого документа относительно конца индексов в файле, не больше 8!
    uint64_t indexNumber : 40; // (5 байт) количество всех индексов файла
} zgdbHeader;

/* Структура для индекса в ZGDB файле */
typedef struct __attribute__((packed)) {
    uint8_t flag; // флаги (т.е. мета-информация об индексе)
    int64_t offset; // смещение блока относительно начала файла
} zgdbIndex;

/* Обёртка для более удобного хранения открытого файла и заголовка вместе */
typedef struct {
    FILE* f; // указатель на открытый файл
    zgdbHeader* header; // указатель на заголовок
} zgdbFile;

/* Флаги для индексов */
typedef enum {
    INDEX_NEW = 1, // индекс только что создан и ещё не привязан к блоку
    INDEX_ALIVE = 2, // индекс привязан к использующемуся ("живому") блоку
    INDEX_DEAD = 3 // индекс привязан к неиспользующемуся ("мертвому") блоку
} indexFlag;

/* Функция для загрузки существующего файла */
zgdbFile* loadFile(const char* filename);

/* Функция для создания нового файла. Также создает список свободных индексов */
zgdbFile* createFile(const char* filename, sortedList* list);

/* Функция для закрытия файла */
void closeFile(zgdbFile* file);

/* Функция для записи заголовка в файл */
bool writeHeader(zgdbFile* file);

/* Функция для записи новых (INDEX_NEW) индексов в файл. Возвращает количество записанных индексов */
size_t writeIndexes(zgdbFile* file, size_t count, sortedList* list);

/* Функция для получения индекса по его порядковому номеру. Возвращает null при неудаче */
zgdbIndex* getIndex(zgdbFile* file, uint64_t i);

/* Функция, меняющая флаг и offset в индексе по его порядковому номеру. Возвращает false при неудаче */
bool updateIndex(zgdbFile* file, uint64_t i, opt_uint8_t flag, opt_int64_t offset);

/* Функция для создания отсортированного списка индексов свободных мест в файле */
sortedList* createList(zgdbFile* file);

#endif