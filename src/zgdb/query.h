#ifndef _QUERY_H_
#define _QUERY_H_

#include <stdbool.h>
#include <stdbool.h>
#include <stdint.h>

#include "element.h"

/* Типы операций. Логические операции применимы ТОЛЬКО к условиям (чтобы делать, например, нечто вроде !cond1 || cond2) */
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

/* Структура для условия. В случае унарной операции (она одна - OP_NOT), второй операнд (condition2) равен
 * первому (condition1) */
typedef struct condition condition;
struct condition {
    bool isMet;
    operationType opType; // тип операции
    union {
        element* el; // элемент для сравнения (если операция - НЕ логическая)
        condition* cond1; // первое условие (если операция - логическая)
    };
    condition* cond2; // второе условие (если операция - логическая и НЕ "!")
};

struct query {
    bool isMutation;
    condition* cond;
    element* newElement;
};

bool checkCondition(element* el, condition* cond);

condition* condEqual(element* el);
condition* condNotEqual(element* el);
condition* condLess(element* el);
condition* condLessOrEqual(element* el);
condition* condGreater(element* el);
condition* condGreaterOrEqual(element* el);
condition* condAnd(condition* cond1, condition* cond2);
condition* condOr(condition* cond1, condition* cond2);
condition* condNot(condition* cond);

#endif