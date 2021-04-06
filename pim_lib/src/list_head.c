#include "list_head.h"

void list_init(list_head_t *list_head) {
    list_head->prev = list_head->next = list_head;
}

int list_empty(list_head_t *list_head) {
    return list_head->next == list_head && list_head->prev == list_head;
}

void list_insert_before(list_head_t *cur, list_head_t *next) {
    cur->next = next;
    cur->prev = next->prev;
    cur->prev->next = next->prev = cur;
}

void list_insert_after(list_head_t *cur, list_head_t *prev) {
    cur->prev = prev;
    cur->next = prev->next;
    cur->next->prev = prev->next = cur;
}

void list_delete(list_head_t *list_head) {
    list_head->prev->next = list_head->next;
    list_head->next->prev = list_head->prev;
}

int list_size(list_head_t *list_head) {
    int cnt = 0;
    list_head_t *cur;

    cur = list_head->next;
    while (cur != list_head) {
        cnt++;
        cur = cur->next;
    }

    return cnt;
}