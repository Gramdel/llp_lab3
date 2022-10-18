#ifndef _BLOCK_H_
#define _BLOCK_H_

#include <stdbool.h>
#include "format.h"

/* Структура для заголовка блока */
typedef struct __attribute__((packed)) documentHeader {
	uint64_t size : 40; // (5 байт) размер блока в байтах
	uint64_t indexOrder : 40; // (5 байт) порядковый номер индекса, прикрепленного к блоку
} documentHeader;

/* Идентификаторы для типов данных в блоке */
typedef enum elementType {
	TYPE_INT = 0x01, // для int32_t
	TYPE_DOUBLE = 0x02, // для double
	TYPE_BOOLEAN = 0x03, // для boolean (uint8_t)
	TYPE_STRING = 0x04, // для строки
	TYPE_EMBEDDED_DOCUMENT = 0x05 // для вложенного документа
} elementType;

/* Терминаторы в блоке */
typedef enum terminator {
	NULL_TERMINATOR = 0x00, // терминатор для строк и ключей в блоке
	DOCUMENT_TERMINATOR = 0xFF, // терминатор для определения границ блоков
	EMBEDDED_DOCUMENT_TERMINATOR = 0xFE // терминатор для вложенного документа
} terminator;

/* Структура для строки */
typedef struct string {
	uint32_t size;
	unsigned char* data;
} string;

typedef struct document document;

/* Структура для элемента блока */
typedef struct element {
	uint8_t type; // тип элемента
	unsigned char key[13]; // ключ элемента
	union {
		int32_t integerValue;
		double doubleValue;
		uint8_t booleanValue;
		string* stringValue; // указатель на строку
		document* documentValue; // указатель на вложенный документ
	} value; // значение элемента
} element;

/* Структура для блока (документа) */
typedef struct document {
	documentHeader header;
	element* elements;
} document;

/* Структура для схемы данных */
typedef struct documentSchema {
	element* elements;
} documentSchema;

/* Функции для добавления данных в схему. Возвращают false при неудаче */
bool addIntegerToSchema(documentSchema* schema, unsigned char* key, int32_t value);

bool addDoubleToSchema(documentSchema* schema, unsigned char* key, double value);

bool addBooleanToSchema(documentSchema* schema, unsigned char* key, uint8_t value);

bool addStringToSchema(documentSchema* schema, unsigned char* key, string* value);

bool addDocumentToSchema(documentSchema* schema, unsigned char* key, document* value);

/* Функция для инициализации схема с определнным количеством элементов */
documentSchema createSchema(uint64_t numberOfElements);

/* Функция для добавления нового (INDEX_NEW) индекса в файл. Возвращает indexNumber из заголовка при неудаче */
uint64_t createIndex(zgdbFile* file);

/* Функция для добавления нового блока в файл. Возвращает false при неудаче */
bool createDocument(zgdbFile* file, documentSchema* schema);

#endif