#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "game_hashtbl.h"

GameHashtbl* game_hashtbl_create(){
    GameHashtbl* tbl = (GameHashtbl*)malloc(sizeof(GameHashtbl));
    if(tbl == NULL) {
        printf("hash table_create fail, malloc table fail\n");
        return NULL;
    }

    tbl->hash_sz = HASH_TABLE_DEFAULT_SIZE;
    tbl->count = 0;
    tbl->min_resize_cnt = (int)(tbl->hash_sz * HASH_TABLE_MIN_LOAD_FACTOR);
    tbl->max_resize_cnt = (int)(tbl->hash_sz * HASH_TABLE_MAX_LOAD_FACTOR);
    tbl->nodes = (GameHashtblNode **)malloc(sizeof(GameHashtblNode*) * tbl->hash_sz);

    if(tbl->nodes == NULL) {
        printf("hash table_create fail, malloc nodes fail\n");
        free(tbl);
        return NULL;
    }

    for (int i = 0; i < tbl->hash_sz; i++)
    {
        tbl->nodes[i] = NULL;
    }

    return tbl;
}

void game_hashtbl_destroy(GameHashtbl *tbl){
    GameHashtblNode *p, *next_p;
    for (int i = 0; i < tbl->hash_sz; i++)
    {
        p = tbl->nodes[i];
        tbl->nodes[i] = NULL;
        while(p!=NULL){
            next_p = p->next;
            free(p);
            p = next_p;
        }
    }

    free(tbl->nodes);
    free(tbl);
}


int game_hashtbl_has(GameHashtbl* tbl, uint64_t key){
    GameHashtblNode* node = tbl->nodes[HASH_TABLE_HASH_FUNC(tbl, key)];
    while(node != NULL){
        if(node->key == key){
            return 1;
        }
        node = node->next;
    }
    return 0;
}


void* game_hashtbl_get(GameHashtbl* tbl, uint64_t key){
    GameHashtblNode* node = tbl->nodes[HASH_TABLE_HASH_FUNC(tbl, key)];
    while(node != NULL){
        if(node->key == key){
            return node->value;
        }
        node = node->next;
    }
    return NULL;
}

static int _hashtbl_insert(GameHashtbl* tbl, uint64_t key, void* value){
    GameHashtblNode* node = (GameHashtblNode*)malloc(sizeof(GameHashtblNode));
    if (node == NULL){
        printf("hash table_insert fail, key<%lu>, malloc fail\n", key);
        return 1;
    }
    node->key = key;
    node->value = value;
    node->next = tbl->nodes[HASH_TABLE_HASH_FUNC(tbl, key)];
    tbl->nodes[HASH_TABLE_HASH_FUNC(tbl, key)] = node;
    tbl->count++;

    if(tbl->hash_sz < HASH_TABLE_MAX_SIZE && tbl->count > tbl->max_resize_cnt){
        game_hashtbl_resize(tbl, tbl->hash_sz * 2);
    }
    return 0;
}


int game_hashtbl_insert(GameHashtbl* tbl, uint64_t key, void* value){
    if(game_hashtbl_has(tbl, key)){
        printf("hash table_insert fail, key<%lu> already exist\n", key);
        return 1;
    }

    return _hashtbl_insert(tbl, key, value);
}

int game_hashtbl_upsert(GameHashtbl* tbl, uint64_t key, void* value){
    GameHashtblNode* node = tbl->nodes[HASH_TABLE_HASH_FUNC(tbl, key)];
    while(node != NULL){
        if(node->key == key){
            node->value = value;
            return 0;
        }
        node = node->next;
    }

    return _hashtbl_insert(tbl, key, value);
}

int game_hashtbl_remove(GameHashtbl* tbl, uint64_t key){
    GameHashtblNode* free_p;
    GameHashtblNode** node = &(tbl->nodes[HASH_TABLE_HASH_FUNC(tbl, key)]);

    while((*node) != NULL){
        if((*node)->key != key){
            node = &((*node)->next);
            continue;
        }

        free_p = *node;
        *node = free_p->next;
        free(free_p);
        free_p = NULL;
        tbl->count--;

        if(tbl->hash_sz > HASH_TABLE_DEFAULT_SIZE && tbl->count < tbl->min_resize_cnt){
            game_hashtbl_resize(tbl, tbl->hash_sz / 2);
        }

        return 0;
    }
    return 1;
}

int game_hashtbl_resize(GameHashtbl *tbl, int new_hash_size) {
    GameHashtblNode **new_nodes = (GameHashtblNode **)malloc(sizeof(GameHashtblNode*) * new_hash_size);
    if(new_nodes == NULL){
        printf("hash table_resize fail, malloc new_nodes fail\n");
        return 1;
    }

    int old_hash_size = tbl->hash_sz;
    tbl->hash_sz = new_hash_size;
    tbl->min_resize_cnt = (int)(tbl->hash_sz * HASH_TABLE_MIN_LOAD_FACTOR);
    tbl->max_resize_cnt = (int)(tbl->hash_sz * HASH_TABLE_MAX_LOAD_FACTOR);

    GameHashtblNode **old_nodes = tbl->nodes;
    for (int i = 0; i < tbl->hash_sz; i++)
    {
        new_nodes[i] = NULL;
    }
    tbl->nodes = new_nodes;

    GameHashtblNode *p, *next_p;
    for (int i = 0; i < old_hash_size; i++)
    {
        p = old_nodes[i];
        old_nodes[i] = NULL;
        while(p != NULL){
            next_p = p->next;
            p->next = new_nodes[HASH_TABLE_HASH_FUNC(tbl, p->key)];
            new_nodes[HASH_TABLE_HASH_FUNC(tbl, p->key)] = p;
            p = next_p;
        }
    }

    free(old_nodes);
    printf("table_resize[%d -> %d], cnt<%d>\n", old_hash_size, tbl->hash_sz, tbl->count);
    return 0;
}

void game_hashtbl_iter_init(GameHashtbl* tbl, GameHashtblIter* iter){
    iter->hash_sz = -1;
    iter->count = tbl->count;
    iter->node = NULL;
}

int game_hashtbl_iter_next(GameHashtbl* tbl, GameHashtblIter* iter){
    if(iter->count <= 0){
        return 0;
    }

    if(iter->node){
        iter->node = iter->node->next;
    }

    while(!iter->node){
        iter->hash_sz++;
        if(iter->hash_sz >= tbl->hash_sz){
            break;
        }
        iter->node = tbl->nodes[iter->hash_sz];
    }

    if(iter->node == NULL){
        return 0;
    }

    iter->count--;
    return 1;
}

void game_hashtbl_foreach(GameHashtbl* tbl, GameHashtblIterFunc func, void *ud){
    for (int i = 0; i < tbl->hash_sz; i++)
    {
        GameHashtblNode* node = tbl->nodes[i];
        while(node != NULL){
            // printf("for<%d>, key<%lu>, value<%p>\n", i, node->key, node->value);
            func(ud, node->key, node->value);
            node = node->next;
        }
    }
    // printf("game_hashtbl_foreach[%d], cnt<%d>\n", tbl->hash_sz, tbl->count);
}

void game_hashtbl_debug(GameHashtbl* tbl){
    for (int i = 0; i < tbl->hash_sz; i++)
    {
        GameHashtblNode* node = tbl->nodes[i];
        while(node != NULL){
            printf("hash<%d>, key<%lu>, value<%p>\n", i, node->key, node->value);
            node = node->next;
        }
    }
    printf("game_hashtbl_debug[%d], cnt<%d>\n", tbl->hash_sz, tbl->count);
}