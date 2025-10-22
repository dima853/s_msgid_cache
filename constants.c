#include "s_msgid_cache.h"

const size_t DEFAULT_CAPACITY = 1000;
const time_t CLEANUP_INTERVAL = 300;   // 5 минут
const time_t ENTRY_TTL = 1800;         // 30 минут
