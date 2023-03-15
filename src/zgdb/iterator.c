#include "iterator.h"
#include "document.h"

#include <malloc.h>
#include <string.h>

iterator* createIterator() {
    iterator* it = malloc(sizeof(iterator));
    if (it) {
        it->refs = NULL;
        it->length = 0;
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
        documentRef* tmp = realloc(it->refs, sizeof(documentRef) * (it->length + 1));
        if (tmp) {
            it->refs = tmp;
            it->refs[it->length++] = ref;
            return true;
        }
    }
    return false;
}

bool addAllRefs(iterator* dest, iterator* src) {
    if (dest) {
        if (!src || !src->length) {
            return true;
        }
        documentRef* tmp = realloc(dest->refs, sizeof(documentRef) * (dest->length + src->length));
        if (tmp) {
            dest->refs = tmp;
            memcpy(dest->refs + dest->length, src->refs, sizeof(documentRef) * src->length);
            dest->length += src->length;
            destroyIterator(src);
            return true;
        }
    }
    return false;
}

bool hasNext(iterator* it) {
    return it && (it->curr + 1 < it->length);
}

document* next(zgdbFile* file, iterator* it) {
    if (hasNext(it)) {
        return readDocument(file, it->refs[++it->curr].indexNumber);
    }
    return NULL;
}