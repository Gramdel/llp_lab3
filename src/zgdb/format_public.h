#ifndef _FORMAT_PUBLIC_H_
#define _FORMAT_PUBLIC_H_

typedef struct zgdbFile zgdbFile;

/* Функция для загрузки существующего файла */
zgdbFile* loadFile(const char* filename);

/* Функция для создания нового файла */
zgdbFile* createFile(const char* filename);

/* Функция для закрытия файла */
void closeFile(zgdbFile* file);

#endif