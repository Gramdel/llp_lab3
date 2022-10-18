#ifndef _FORMAT_H_
#define _FORMAT_H_

#include <stdint.h>
#include <stdio.h>

/* Структура для заголовка ZGDB файла */
typedef struct __attribute__((packed)) zgdbHeader {
	uint32_t fileType; // должны быть записаны 4 буквы в UTF-8: ZGDB
	uint64_t indexNumber : 40; // (5 байт) количество всех индексов файла
} zgdbHeader;

/* Обёртка для более удобного хранения открытого файла и заголовка вместе */
typedef struct zgdbFile {
	FILE* file; // указатель на открытый файл
	zgdbHeader* header; // указатель на заголовок
} zgdbFile;

/* Функция для загрузки или создания файла */
zgdbFile* loadOrCreateFile(const char* fileName);

/* Функция для закрытия файла */
uint8_t closeFile(zgdbFile* file);

/* Функция для сохранения заголовка */
uint8_t saveHeader(zgdbFile* file);

#endif