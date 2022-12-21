#ifndef _SCHEMA_H_
#define _SCHEMA_H_

#include "schema_public.h"
#include "element_public.h"

struct documentSchema{
    element* elements;
    uint64_t elementCount;
    uint64_t capacity;
};

#endif