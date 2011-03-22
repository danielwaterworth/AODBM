/*  
    aodbm - Append Only Database Manager
    Copyright (C) 2011 Daniel Waterworth

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "aodbm_list.h"
#include "aodbm_error.h"

#include "stdlib.h"

struct list_node {
    struct list_node *next, *prev;
    void *item;
};

typedef struct list_node list_node;

struct aodbm_list {
    list_node *head;
    list_node *tail;
    size_t len;
};

struct aodbm_list_iterator {
    list_node *cur;
    aodbm_list *list;
};

static list_node *create_list_node(void *item) {
    list_node *node = malloc(sizeof(list_node));
    node->item = item;
    return node;
}

aodbm_list *aodbm_list_empty() {
    aodbm_list *list = malloc(sizeof(aodbm_list));
    list->head = NULL;
    list->tail = NULL;
    list->len = 0;
    return list;
}

void aodbm_free_list(aodbm_list *list) {
    while(!aodbm_list_is_empty(list)) {
        void *item = aodbm_list_pop_back(list);
        if (item != NULL) {
            free(item);
        }
    }
    free(list);
}

void aodbm_list_push_front(aodbm_list *list, void *item) {
    list_node *node = create_list_node(item);
    if (list->head == NULL) {
        node->next = NULL;
        node->prev = NULL;
        list->head = node;
        list->tail = node;
    } else {
        list->head->prev = node;
        node->next = list->head;
        node->prev = NULL;
        list->head = node;
    }
    list->len += 1;
}

void aodbm_list_push_back(aodbm_list *list, void *item) {
    list_node *node = create_list_node(item);
    if (list->head == NULL) {
        node->next = NULL;
        node->prev = NULL;
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        node->prev = list->tail;
        node->next = NULL;
        list->tail = node;
    }
    list->len += 1;
}

void *aodbm_list_pop_front(aodbm_list *list) {
    list_node *head = list->head;
    if (head == NULL) {
        return NULL;
    }
    void *out = head->item;
    list->head = head->next;
    if (list->head != NULL) {
        list->head->prev = NULL;
    } else {
        list->tail = NULL;
    }
    free(head);
    list->len -= 1;
    return out;
}

void *aodbm_list_pop_back(aodbm_list *list) {
    list_node *tail = list->tail;
    if (tail == NULL) {
        return NULL;
    }
    void *out = tail->item;
    list->tail = tail->prev;
    if (list->tail != NULL) {
        list->tail->next = NULL;
    } else {
        list->head = NULL;
    }
    free(tail);
    list->len -= 1;
    return out;
}

bool aodbm_list_is_empty(aodbm_list *list) {
    return list->head == NULL;
}

size_t aodbm_list_length(aodbm_list *list) {
    return list->len;
}

aodbm_list *aodbm_list_merge_di(aodbm_list *a, aodbm_list *b) {
    if (a->tail != NULL) {
        a->tail->next = b->head;
    }
    if (b->head != NULL) {
        b->head->prev = a->tail;
    }
    if (a->head == NULL) {
        a->head = b->head;
    }
    if (b->tail != NULL) {
        a->tail = b->tail;
    }
    a->len += b->len;
    free(b);
    return a;
}

aodbm_list_iterator *aodbm_list_begin(aodbm_list *list) {
    aodbm_list_iterator *it = malloc(sizeof(aodbm_list_iterator));
    
    it->list = list;
    it->cur = list->head;
    
    return it;
}

aodbm_list_iterator *aodbm_list_back(aodbm_list *list) {
    aodbm_list_iterator *it = malloc(sizeof(aodbm_list_iterator));
    
    it->list = list;
    it->cur = list->tail;
    
    return it;
}

void aodbm_list_iterator_next(aodbm_list_iterator *it) {
    if (it->cur == NULL) {
        return;
    }
    it->cur = it->cur->next;
}

void aodbm_list_iterator_prev(aodbm_list_iterator *it) {
    if (it->cur == NULL) {
        return;
    }
    it->cur = it->cur->prev;
}

bool aodbm_list_iterator_is_begin(aodbm_list_iterator *it) {
    return it->cur == it->list->head;
}

bool aodbm_list_iterator_is_end(aodbm_list_iterator *it) {
    return it->cur == it->list->tail;
}

bool aodbm_list_iterator_is_finished(aodbm_list_iterator *it) {
    return it->cur == NULL;
}

void *aodbm_list_iterator_get(aodbm_list_iterator *it) {
    return it->cur->item;
}

void aodbm_list_insert(aodbm_list_iterator *it, void *item) {
    list_node *node = create_list_node(item);
    if (it->cur != NULL) {
        list_node *next = it->cur->next;
        list_node *prev = it->cur;
        if (next != NULL) {
            next->prev = node;
        }
        node->next = next;
        node->prev = prev;
        prev->next = node;
    } else {
        if (it->list->head != NULL) {
            AODBM_CUSTOM_ERROR("unknown location");
        } else {
            it->list->head = node;
            it->list->tail = node;
            node->next = NULL;
            node->prev = NULL;
        }
    }
    it->list->len += 1;
}

void aodbm_list_remove(aodbm_list_iterator *it) {
    if (it->cur != NULL) {
        list_node *next = it->cur->next;
        list_node *prev = it->cur->prev;
        if (prev != NULL) {
            prev->next = next;
        } else {
            it->list->head = next;
        }
        if (next != NULL) {
            next->prev = prev;
        } else {
            it->list->tail = prev;
        }
        free(it->cur);
        if (next != NULL) {
            it->cur = next;
        } else {
            it->cur = prev;
        }
        it->list->len -= 1;
    }
}

void aodbm_free_list_iterator(aodbm_list_iterator *it) {
    free(it);
}
