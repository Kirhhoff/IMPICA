typedef struct list_head {
    struct list_head* prev;
    struct list_head* next;
} list_head_t;

void list_init(list_head_t *list_head);
int list_empty(list_head_t *list_head);
void list_insert_before(list_head_t *cur, list_head_t *next);
void list_insert_after(list_head_t *cur, list_head_t *prev);
void list_delete(list_head_t *list_head);
int list_size(list_head_t *list_head);