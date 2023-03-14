#include "iterator.h"
#include "document.h"

#include <malloc.h>

iterator* createIterator() {
    iterator* it = malloc(sizeof(iterator));
    if (it) {
        it->refs = NULL;
        it->size = 0;
        it->curr = -1;
    }
    return it;
}

void destroyIterator(iterator* it) {
    if (it) {
        if (it->refs) {
            free(it->refs);
        }
        free(it);
    }
}

bool addRef(iterator* it, documentRef ref) {
    if (it) {
        documentRef* tmp = realloc(it->refs, sizeof(documentRef) * (it->size + 1));
        if (tmp) {
            it->refs = tmp;
            it->refs[it->size++] = ref;
            return true;
        }
    }
    return false;
}

bool hasNext(iterator* it) {
    return it && (it->curr + 1 < it->size);
}

document* next(zgdbFile* file, iterator* it) {
    if (hasNext(it)) {
        return readDocument(file, it->refs[++it->curr].indexNumber);
    }
    return NULL;
}