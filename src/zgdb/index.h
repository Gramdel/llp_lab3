#ifndef _INDEX_H_
#define _INDEX_H_

#include <stdbool.h>
#include "format.h"

/* Структура для id, привязанного к блоку */
typedef struct __attribute__((packed)) blockId {
	uint32_t timestamp; // время создания блока в секундах с эпохи UNIX
	uint64_t offset : 40; // (5 байт) смещение блока относительно конца индексов в файле на момент создания блока
} blockId;

/* Флаги для индексов */
typedef enum indexFlag {
	INDEX_NEW = 0, // индекс только что создан и ещё не привязан к блоку
	INDEX_ALIVE = 1, // индекс привязан к использующемуся ("живому") блоку
	INDEX_DEAD = 2 // индекс привязан к неиспользующемуся ("мертвому") блоку
} indexFlag;

/* Структура для индекса в ZGDB файле */
typedef struct __attribute__((packed)) zgdbIndex {
	uint8_t flags; // флаги (т.е. мета-информация об индексе)
	blockId id; // id, привязанный к блоку
	uint64_t offset : 40; // смещение блока относительно конца индексов в файле
} zgdbIndex;

/* Функция для получения индекса по его порядковому номеру. Возвращает null при неудаче */
zgdbIndex* getIndexByOrder(zgdbFile* file, uint64_t i);

/* Функция для получения индекса, содержащего конкретный id. Возвращает null при неудаче */
zgdbIndex* getIndexById(zgdbFile* file, blockId id);

/* Функция для получения номера индекса, содержащего конкретный id. Возвращает indexNumber из заголовка при неудаче */
uint64_t getIndexOrderById(zgdbFile* file, blockId id);

/* Функция, помечающая индекс как INDEX_DEAD по его порядковому номеру. Возвращает false при неудаче */
bool killIndexByOrder(zgdbFile* file, uint64_t i);

/* Функция, помечающая индекс, содержащий конкретный id, как INDEX_DEAD. Возвращает false при неудаче */
bool killIndexById(zgdbFile* file, blockId id);

/* Функция, "прикрепляющая" блок к INDEX_DEAD или INDEX_NEW индексу и делающая его INDEX_ALIVE. Сначала проверяет наличие блока. Возвращает false при неудаче */
bool attachBlockToIndex(zgdbFile* file, uint64_t order, blockId id, uint64_t offset);

/* Функция, меняющая offset в INDEX_ALIVE индексе. При неудаче возвращает false */
bool changeOffsetByOrder(zgdbFile* file, uint64_t order, uint64_t offset);

#endif