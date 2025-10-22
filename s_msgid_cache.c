#include "s_msgid_cache.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static struct s_msgid_cache *global_cache = NULL;

int s_msgid_cache_init(void) {
    if (global_cache != NULL) {
        printf("[MSGID_CACHE] Already initialized\n");
        return 0;
    }
    
    global_cache = calloc(1, sizeof(struct s_msgid_cache));
    if (!global_cache) {
        fprintf(stderr, "[MSGID_CACHE] Failed to allocate cache structure\n");
        return -1;
    }
    
    global_cache->capacity = DEFAULT_CAPACITY;
    global_cache->count = 0;
    
    atomic_store(&global_cache->state, CACHE_STATE_EMPTY);
    atomic_store(&global_cache->total_operations, 0);
    atomic_store(&global_cache->add_operations, 0);
    atomic_store(&global_cache->check_operations, 0);
    atomic_store(&global_cache->replay_detected, 0);
    
    global_cache->entries = calloc(DEFAULT_CAPACITY, sizeof(struct s_msgid_cache_entry));
    if (!global_cache->entries) {
        fprintf(stderr, "[MSGID_CACHE] Failed to allocate entries array\n");
        free(global_cache);
        global_cache = NULL;
        return -1;
    }
    
    time_t now = time(NULL);
    global_cache->created_at = now;
    global_cache->last_cleanup = now;
    
    printf("[MSGID_CACHE] Initialized with capacity: %zu\n", global_cache->capacity);
    return 0;
}

int s_msgid_cache_add(uint32_t msgid, uint32_t source_ip) {
    if (!global_cache) {
        return -1; 
    }
    
    if (atomic_load(&global_cache->state) == CACHE_STATE_ERROR) {
        return -1; 
    }
    
    if (global_cache->count >= global_cache->capacity) {
        s_msgid_cache_cleanup_old(); // чистим устаревшие
    }
    
    global_cache->entries[global_cache->count].msgid = msgid;
    global_cache->entries[global_cache->count].timestamp = time(NULL);
    global_cache->entries[global_cache->count].source_ip = source_ip;
    global_cache->count++;
    
    atomic_fetch_add(&global_cache->total_operations, 1);
    atomic_fetch_add(&global_cache->add_operations, 1);
    
    if (atomic_load(&global_cache->total_operations) % 100 == 0) {
        s_msgid_cache_cleanup_old();
    }
    
    return 0; 
}
