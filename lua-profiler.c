#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

#include <lua.h>
#include <lauxlib.h>

#include "game_hashtbl.h"


#ifdef _WIN32
#include <windows.h>

static uint64_t get_high_precision_time() {
    FILETIME filetime;
    GetSystemTimeAsFileTime(&filetime);

    ULARGE_INTEGER uli;
    uli.LowPart = filetime.dwLowDateTime;
    uli.HighPart = filetime.dwHighDateTime;

    uint64_t time = uli.QuadPart;
    return time;
}

static inline double realtime(uint64_t t) {
    return (double) t / (10000000.0);
}

#elif RDTSC
#include "rdtsc.h"
static inline uint64_t get_high_precision_time() {
    return rdtsc();
}

static inline double realtime(uint64_t t) {
    return (double) t / (2000000000.0);
}

#elif defined(__unix__) || defined(__APPLE__)

#include <sys/time.h>
static uint64_t get_high_precision_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t time = tv.tv_sec * 1000000 + tv.tv_usec;
    return time;
}

static inline double realtime(uint64_t t) {
    return (double) t / (1000000.0);
}

#else

static uint64_t get_high_precision_time() {
    return -1;
}

static inline double realtime(uint64_t t) {
    return -1;
}

#endif

typedef struct {
    int call_count;
    uint64_t total_time;
    char name[128];
    char source[128];
    int line;
} profile_record;

struct call_item {
    const void* point;
    const char* name;
    const char* source;
    int line;
    uint64_t call_time;
    uint64_t sub_cost;
    struct call_item* next;
};

struct call_stack {
    struct call_item* head;
    int count;
};

static struct call_stack* CALL_STACK = NULL;
static GameHashtbl *hashtbl = NULL;

static struct call_stack* stack_create() {
    struct call_stack* stack = (struct call_stack*)malloc(sizeof(struct call_stack));
    stack->head = NULL;
    stack->count = 0;
    return stack;
}

static void push_call_item(struct call_item* item) {
    CALL_STACK->count++;
    item->next = CALL_STACK->head;
    CALL_STACK->head = item;
}

static struct call_item* pop_call_item() {
    struct call_item* item = CALL_STACK->head;
    if(item == NULL){
        return NULL;
    }
    CALL_STACK->head = item->next;
    CALL_STACK->count--;
    return item;
}

static void profiler_hook(lua_State *L, lua_Debug *arv) {
    lua_Debug ar;
    lua_getstack(L, 0, &ar);
    lua_getinfo(L, "nSlf", &ar);

    if (arv->event == LUA_HOOKCALL) {
        const void* point = lua_topointer(L, -1);
        uint64_t now = get_high_precision_time();
        struct call_item* item = (struct call_item*)malloc(sizeof(struct call_item));
        item->point = point;
        item->name = ar.name ? ar.name : "?";
        item->source = ar.source ? ar.source : "?";
        item->line = ar.currentline;
        item->call_time = now;
        item->sub_cost = 0;
        push_call_item(item);
    }
    else if (arv->event == LUA_HOOKRET) {
        struct call_item* item = pop_call_item();
        if (item == NULL) {
            return;
        }
        uint64_t now = get_high_precision_time();
        uint64_t cost = now - item->call_time;
        uint64_t key = (uint64_t)((uintptr_t)item->point);
        profile_record* record = (profile_record*)game_hashtbl_get(hashtbl, key);
        if(record==NULL){
            record = (profile_record*)malloc(sizeof(profile_record));
            record->call_count = 1;
            record->total_time = cost - item->sub_cost;
            record->line = item->line;
            strncpy(record->name, item->name, sizeof(record->name));
            strncpy(record->source, item->source, sizeof(record->source));
            game_hashtbl_insert(hashtbl, key, record);
        }else{
            record->call_count++;
            record->total_time += cost - item->sub_cost;
        }
        if(CALL_STACK->head){
            CALL_STACK->head->sub_cost += cost;
        }
        free(item);
    }
}

static int _start(lua_State *L) {
    if (CALL_STACK) {
        assert(0);
    }
    CALL_STACK = stack_create();
    hashtbl = game_hashtbl_create();
    if(hashtbl == NULL){
        printf("hashtbl create failed\n");
        return 0;
    }
    lua_sethook(L, profiler_hook, LUA_MASKCALL | LUA_MASKRET, 0);
    return 0;
}

static int _stop(lua_State *L) {
    lua_sethook(L, NULL, 0, 0);

    uint64_t total_time = 0;
    for(int i=0; i<hashtbl->hash_sz; i++){
        GameHashtblNode* node = hashtbl->nodes[i];
        while(node != NULL){
            profile_record* record = (profile_record*)node->value;
            total_time += record->total_time;
            node = node->next;
        }
    }

    for(int i=0; i<hashtbl->hash_sz; i++){
        GameHashtblNode* node = hashtbl->nodes[i];
        while(node != NULL){
            profile_record* record = (profile_record*)node->value;
            printf("%f (%02.2f%%)  %10d  %20s  %s:%d\n", realtime(record->total_time),
                record->total_time*100.0/total_time,
                record->call_count, record->name,  record->source, record->line);
            node = node->next;
        }
    }
    printf("total time is   %f\n", realtime(total_time));

    GameHashtblIter iter;
    game_hashtbl_iter_init(hashtbl, &iter);
    while(game_hashtbl_iter_next(hashtbl, &iter)) {
        profile_record* record = (profile_record*)iter.node->value;
        iter.node->value = NULL;
        free(record);
    }
    game_hashtbl_destroy(hashtbl);

    struct call_item* item = CALL_STACK->head;
    while (item) {
        struct call_item* next = item->next;
        free(item);
        item = next;
    }
    free(CALL_STACK);
    CALL_STACK = NULL;

    return 0;
}


int luaopen_profiler(lua_State *L) {
    luaL_checkversion(L);
    luaL_Reg l[] = {
        {"start", _start},
        {"stop", _stop},
        {NULL, NULL},
    };
    luaL_newlib(L, l);
    return 1;
}