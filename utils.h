#ifndef UTILS
#define UTILS
#endif // !UTILS

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef UTILS_IMPLEMENTATION

#ifndef HT_INIT_CAP
#define HT_INIT_CAP 64
#endif // ! HT_INIT_CAP

#ifndef HT_BASE
#define HT_BASE 0x811c9dc5
#endif // !HT_BASE

#ifndef HT_PRIME

#define HT_PRIME 0x01000193
#endif // !HT_PRIME

#define ht_hash(ht, str)                                                       \
  ({                                                                           \
    size_t h = HT_BASE;                                                        \
    const char *s = (str);                                                     \
    while (*s) {                                                               \
      h ^= *s++;                                                               \
      h *= HT_PRIME;                                                           \
    }                                                                          \
    h &((ht)->capacity - 1);                                                   \
  })

#define ht_init(ht, cap)                                                       \
  do {                                                                         \
    (ht)->count = 0;                                                           \
    (ht)->capacity = (cap) ? (cap) : HT_INIT_CAP;                              \
    (ht)->buckets = calloc((ht)->capacity, sizeof(void *));                    \
    assert((ht)->buckets != NULL && "Buy more RAM lol");                       \
  } while (0)

#define ht_put(ht, str, value)                                                 \
  do {                                                                         \
    if ((ht)->count * 10 / (ht)->capacity > 7) {                               \
      ht_rehash(ht);                                                           \
    }                                                                          \
    size_t index = ht_hash(ht, str);                                           \
    (ht)->buckets[index] = (value);                                            \
    (ht)->count++;                                                             \
  } while (0)

#define ht_get_str(ht, str)                                                    \
  ({                                                                           \
    size_t index = ht_hash(ht, str);                                           \
    (char *)(ht)->buckets[index];                                              \
  })

#define ht_get(ht, str)                                                        \
  ({                                                                           \
    size_t index = ht_hash(ht, str);                                           \
    (ht)->buckets[index];                                                      \
  })

#define ht_rehash(ht)                                                          \
  do {                                                                         \
    size_t old_capacity = (ht)->capacity;                                      \
    void **old_buckets = (ht)->buckets;                                        \
    (ht)->capacity *= 2;                                                       \
    (ht)->buckets = calloc((ht)->capacity, sizeof(*(ht)->buckets));            \
    assert((ht)->buckets != NULL && "Buy more RAM lol");                       \
    for (size_t i = 0; i < old_capacity; i++) {                                \
      if (old_buckets[i] != NULL) {                                            \
        (ht)->buckets[i] = old_buckets[i];                                     \
      }                                                                        \
    }                                                                          \
    free(old_buckets);                                                         \
  } while (0);

#define ht_delete(ht, str)                                                     \
  do {                                                                         \
    size_t index = ht_hash(ht, str);                                           \
    if ((ht)->buckets[index] != NULL) {                                        \
      (ht)->buckets[index] = NULL;                                             \
      (ht)->count--;                                                           \
    }                                                                          \
  } while (0);

#define ht_has(ht, str)                                                        \
  ({                                                                           \
    size_t index = ht_hash(ht, str);                                           \
    (ht)->buckets[index] != NULL;                                              \
  })

#define ht_free(ht)                                                            \
  do {                                                                         \
    free((ht)->buckets);                                                       \
    (ht)->buckets = NULL;                                                      \
    (ht)->count = 0;                                                           \
    (ht)->capacity = 0;                                                        \
  } while (0)

#ifndef DA_INIT_CAP
#define DA_INIT_CAP 256
#endif // !DA_INIT_CAP

#define da_reserve(da, expected_capacity)                                      \
  do {                                                                         \
    if ((expected_capacity) > (da)->capacity) {                                \
      if ((da)->capacity == 0) {                                               \
        (da)->capacity = DA_INIT_CAP;                                          \
      }                                                                        \
      while ((expected_capacity) > (da)->capacity) {                           \
        (da)->capacity *= 2;                                                   \
      }                                                                        \
      (da)->items =                                                            \
          realloc((da)->items, (da)->capacity * sizeof(*(da)->items));         \
      assert((da)->items != NULL && "Buy more RAM lol");                       \
    }                                                                          \
  } while (0)

#define da_append(da, item)                                                    \
  do {                                                                         \
    da_reserve((da), (da)->count + 1);                                         \
    (da)->items[(da)->count++] = (item);                                       \
  } while (0)

#define da_foreach(Type, it, da)                                               \
  for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

#define da_delete_at(da, i)                                                    \
  do {                                                                         \
    size_t index = (i);                                                        \
    assert(index < (da)->count);                                               \
    memmove(&(da)->items[index], &(da)->items[index + 1],                      \
            ((da)->count - index - 1) * sizeof(*(da)->items));                 \
    (da)->count -= 1;                                                          \
  } while (0)

#define sb_append_null(sb) da_append(sb, 0)

#endif // !UTILS_IMPLEMENTATION
