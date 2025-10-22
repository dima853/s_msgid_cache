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

void s_msgid_cache_cleanup_old(void) {
    if (!global_cache || !global_cache->entries) return;
    
    time_t now = time(NULL);
    size_t new_count = 0;
    
    for (size_t i = 0; i < global_cache->count; i++) {
        if ((now - global_cache->entries[i].timestamp) <= ENTRY_TTL) {
            if (new_count != i) {
                global_cache->entries[new_count] = global_cache->entries[i];
            }
            new_count++;
        }
    }
    
    size_t removed_count = global_cache->count - new_count;
    global_cache->count = new_count;
    
    global_cache->last_cleanup = now;
    
    if (removed_count > 0) {
        printf("[CACHE] Cleaned %zu expired entries, now %zu active\n", 
               removed_count, new_count);
    }
}

void s_msgid_cache_destroy(void) {
    if (global_cache == NULL) {
        return NULL;
    }

    if (global_cache->entries) {
        free(global_cache->entries);
        global_cache->entries = NULL;
    }

    free(global_cache);
    global_cache = NULL;
}


