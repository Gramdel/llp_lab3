#include <malloc.h>

#include "list.h"

listNode* createNode(uint64_t size, uint64_t index) {
    listNode* node = malloc(sizeof(listNode));
    if (node) {
        node->size = size;
        node->index = index;
        node->prev = NULL;
        node->next = NULL;
    }
    return node;
}

sortedList* createList() {
    sortedList* list = malloc(sizeof(sortedList));
    if (list) {
        list->front = NULL;
        list->back = NULL;
    }
    return list;
}

void destroyList(sortedList* list) {
    if (list) {
        listNode* curr = list->front;
        while (curr) {
            listNode* next = curr->next;
            free(curr);
            curr = next;
        }
        free(list);
    }
}

void insertNode(sortedList* list, listNode* node) {
    if (list->front) {
        listNode* curr = list->front;
        while (curr) {
            if (node->size > curr->size) {
                node->next = curr;
                curr->prev = node;
                if (curr == list->front) {
                    list->front = node;
                }
                break;
            } else if (curr == list->back) {
                node->prev = curr;
                curr->next = node;
                list->back = node;
                break;
            }
            curr = curr->next;
        }
    } else {
        list->front = node;
        list->back = node;
    }
}

listNode* popFront(sortedList* list) {
    if (list) {
        if (list->front) {
            listNode* front = list->front;
            list->front = list->front->next;
            if (list->front) {
                list->front->prev = NULL;
            } else {
                list->back = NULL;
            }
            return front;
        }
    }
    return NULL;
}

listNode* popBack(sortedList* list) {
    if (list) {
        if (list->back) {
            listNode* back = list->back;
            list->back = list->back->prev;
            if (list->back) {
                list->back->next = NULL;
            } else {
                list->front = NULL;
            }
            return back;
        }
    }
    return NULL;
}