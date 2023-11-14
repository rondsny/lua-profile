#include <stdint.h>

#define HASH_TABLE_DEFAULT_SIZE 16
#define HASH_TABLE_MAX_SIZE 1024
#define HASH_TABLE_MIN_LOAD_FACTOR 0.25
#define HASH_TABLE_MAX_LOAD_FACTOR 0.75
#define HASH_TABLE_HASH_FUNC(tbl, key) ((key) % (tbl)->hash_sz)

typedef struct game_hashtbl_node {
    uint64_t key;
    void *value;
    struct game_hashtbl_node* next;
} GameHashtblNode;

typedef struct game_bashtbl {
    int hash_sz;
    int count;
    int max_resize_cnt; // max count of resize
    int min_resize_cnt;
    GameHashtblNode** nodes; // array of node ptr
} GameHashtbl;

typedef struct game_hashtbl_iter {
    int hash_sz;
    int count;
    GameHashtblNode *node;
} GameHashtblIter;

typedef void (*GameHashtblIterFunc)(void *ud, uint64_t key, void *value);

GameHashtbl* game_hashtbl_create();

void game_hashtbl_destroy(GameHashtbl *tbl);

int game_hashtbl_has(GameHashtbl* tbl, uint64_t key);

void* game_hashtbl_get(GameHashtbl* tbl, uint64_t key);

int game_hashtbl_insert(GameHashtbl* tbl, uint64_t key, void* value);

int game_hashtbl_upsert(GameHashtbl* tbl, uint64_t key, void* value); // update or insert

int game_hashtbl_remove(GameHashtbl* tbl, uint64_t key);

int game_hashtbl_resize(GameHashtbl *tbl, int new_hash_size);

void game_hashtbl_iter_init(GameHashtbl* tbl, GameHashtblIter* iter);

int game_hashtbl_iter_next(GameHashtbl* tbl, GameHashtblIter* iter);

void game_hashtbl_foreach(GameHashtbl* tbl, GameHashtblIterFunc func, void *ud);

void game_hashtbl_debug(GameHashtbl* tbl);
