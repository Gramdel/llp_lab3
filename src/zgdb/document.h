#ifndef _DOCUMENT_H_
#define _DOCUMENT_H_

#include <stdbool.h>
#include "format.h"

/* Структура для id, привязанного к документу (здесь и далее блок == документ). Для вложенных - id нулевой */
typedef struct __attribute__((packed)) documentId {
	uint32_t timestamp; // время создания документа в секундах с эпохи UNIX
	uint64_t offset; // смещение документа относительно начала файла на момент создания документа
} documentId;

/* Структура для заголовка документа */
typedef struct __attribute__((packed)) documentHeader {
	uint64_t size : 40; // (5 байт) размер документа в байтах
	union {
		uint64_t indexOrder : 40; // (5 байт) порядковый номер индекса, прикрепленного к документу
		uint64_t internalOffset : 40; // (5 байт) смещение вложенного документа относительного родительского
	};
	documentId id; // id, привязанный к документу
} documentHeader;

/* Идентификаторы для типов данных в документе */
typedef enum elementType {
	TYPE_INT = 0x01, // для int32_t
	TYPE_DOUBLE = 0x02, // для double
	TYPE_BOOLEAN = 0x03, // для boolean (uint8_t)
	TYPE_STRING = 0x04, // для строки
	TYPE_EMBEDDED_DOCUMENT = 0x05 // для вложенного документа
} elementType;

/* Терминаторы в документе */
typedef enum terminator {
	NULL_TERMINATOR = 0x00, // терминатор для строк и ключей в документе
	DOCUMENT_TERMINATOR = 0xFF, // терминатор для определения границ документов
	EMBEDDED_DOCUMENT_TERMINATOR = 0xFE // терминатор для вложенного документа
} terminator;

/* Структура для строки */
typedef struct string {
	uint32_t size;
	unsigned char* data;
} string;

typedef struct document document;

/* Структура для элемента документа */
typedef struct element {
	uint8_t type; // тип элемента
	unsigned char key[13]; // ключ элемента
	union {
		int32_t integerValue;
		double doubleValue;
		uint8_t booleanValue;
		string* stringValue; // указатель на строку
		document* documentValue; // указатель на вложенный документ
	};
} element;

/* Структура для документа (документа) */
typedef struct document {
	documentHeader header;
	element* elements;
	size_t elementNumber;
} document;

/* Структура для схемы данных */
typedef struct documentSchema {
	element* elements;
	size_t elementNumber;
} documentSchema;

/* Функции для добавления данных в схему. Возвращают false при неудаче */
bool addIntegerToSchema(documentSchema* schema, unsigned char* key, int32_t value);

bool addDoubleToSchema(documentSchema* schema, unsigned char* key, double value);

bool addBooleanToSchema(documentSchema* schema, unsigned char* key, uint8_t value);

bool addStringToSchema(documentSchema* schema, unsigned char* key, string* value);

bool addDocumentToSchema(documentSchema* schema, unsigned char* key, document* value);

/* Функция для инициализации схемы с определнным количеством элементов */
documentSchema createSchema(size_t elementNumber);

/* Функция для добавления нового (INDEX_NEW) индекса в файл. Возвращает indexNumber из заголовка при неудаче */
uint64_t createIndex(zgdbFile* file);

/* Функция для добавления нового документа в файл. Возвращает false при неудаче */
bool createDocument(zgdbFile* file, documentSchema* schema);

#endif