#ifndef _QUERY_H_
#define _QUERY_H_

#include <stdbool.h>
#include <stdbool.h>
#include <stdint.h>

#include "element.h"

// Типы операций. Логические операции применимы ТОЛЬКО к условиям (чтобы делать, например, нечто вроде !cond1 || cond2)
typedef enum operationType {
    OP_EQ = 0, // операция "=="
    OP_NEQ, // операция "!="
    OP_GT, // операция ">"
    OP_GTE, // операция ">="
    OP_LE, // операция "<"
    OP_LEE, // операция "<="
    OP_AND, // операция "&&"
    OP_OR, // операция "||"
    OP_NOT, // операция "!"
} operationType;

// Структура для условия.
typedef struct condition condition;
struct condition {
    operationType opType; // тип операции
    union {
        element* element1; // первый элемент для сравнения (если операция - НЕ логическая)
        condition* condition1; // первое условие для сравнения (если операция - логическая)
    };
    union {
        element* element2; // второй элемент для сравнения (если операция - НЕ логическая)
        condition* condition2; // второе условие для сравнения (если операция - логическая и НЕ "!")
    };
};

struct query {
    bool isMutation;

};

#endif