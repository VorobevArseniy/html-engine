#ifndef UTILS
#define UTILS
#endif // !UTILS

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
//   https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
#ifdef __MINGW_PRINTF_FORMAT
#define PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)                            \
  __attribute__((format(__MINGW_PRINTF_FORMAT, STRING_INDEX, FIRST_TO_CHECK)))
#else
#define PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)                            \
  __attribute__((format(printf, STRING_INDEX, FIRST_TO_CHECK)))
#endif // __MINGW_PRINTF_FORMAT
#else
//   TODO: implement PRINTF_FORMAT for MSVC
#define PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)
#endif

#ifndef UTILS_IMPLEMENTATION

#define UNUSED(value) (void)(value)
#define TODO(message)                                                          \
  do {                                                                         \
    fprintf(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, message);         \
    abort();                                                                   \
  } while (0)
#define UNREACHABLE(message)                                                   \
  do {                                                                         \
    fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message);  \
    abort();                                                                   \
  } while (0)

typedef struct Entry {
  char *key;
  void *value;
} Entry;

typedef struct Map {
  size_t count;
  size_t capacity;

  Entry **buckets;
} Map;

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
    (ht)->buckets = calloc((ht)->capacity, sizeof(Entry *));                   \
    assert((ht)->buckets != NULL && "Buy more RAM lol");                       \
  } while (0)

#define ht_put(ht, str, val)                                                   \
  do {                                                                         \
    if ((ht)->count * 10 / (ht)->capacity > 7) {                               \
      ht_rehash(ht);                                                           \
    }                                                                          \
    size_t index = ht_hash(ht, str);                                           \
                                                                               \
    if ((ht)->buckets[index] != NULL) {                                        \
      free((ht)->buckets[index]->key);                                         \
      free((ht)->buckets[index]);                                              \
      (ht)->buckets[index] = NULL;                                             \
    }                                                                          \
    Entry *entry = malloc(sizeof(Entry));                                      \
    entry->key = strdup(str);                                                  \
    entry->value = (val);                                                      \
    (ht)->buckets[index] = entry;                                              \
    (ht)->count++;                                                             \
  } while (0)

#define ht_get_str(ht, str)                                                    \
  ({                                                                           \
    size_t index = ht_hash(ht, str);                                           \
    Entry *entry = (ht)->buckets[index];                                       \
    void *result = NULL;                                                       \
    if (enrty && strcmp(entry->key, str) == 0) {                               \
      result = (char *)entry->value;                                           \
    }                                                                          \
    result;                                                                    \
  })

#define ht_get(ht, str)                                                        \
  ({                                                                           \
    size_t index = ht_hash(ht, str);                                           \
    Entry *entry = (ht)->buckets[index];                                       \
    void *result = NULL;                                                       \
    if (enrty && strcmp(entry->key, str) == 0) {                               \
      result = entry->value;                                                   \
    }                                                                          \
    result;                                                                    \
  })

#define ht_rehash(ht)                                                          \
  do {                                                                         \
    size_t old_capacity = (ht)->capacity;                                      \
    Entry **old_buckets = (ht)->buckets;                                       \
    (ht)->capacity *= 2;                                                       \
    (ht)->buckets = calloc((ht)->capacity, sizeof(Entry *));                   \
    assert((ht)->buckets != NULL && "Buy more RAM lol");                       \
    for (size_t i = 0; i < old_capacity; i++) {                                \
      if (old_buckets[i] != NULL) {                                            \
        size_t new_index = ht_hash(ht, old_buckets[i]->key);                   \
        if ((ht)->buckets[new_index]->key) {                                   \
          free((ht)->buckets[new_index]->key);                                 \
          free((ht)->buckets[new_index]);                                      \
        }                                                                      \
        (ht)->buckets[new_index] = old_buckets[i];                             \
      }                                                                        \
    }                                                                          \
    free(old_buckets);                                                         \
  } while (0)

#define ht_delete(ht, str)                                                     \
  do {                                                                         \
    size_t index = ht_hash(ht, str);                                           \
    if ((ht)->buckets[index] != NULL &&                                        \
        strcmp((ht)->buckets[index]->key, str == 0)) {                         \
      free((ht)->buckets[index]->key);                                         \
      free((ht)->buckets[index]);                                              \
      (ht)->buckets[index] = NULL;                                             \
      (ht)->count--;                                                           \
    }                                                                          \
  } while (0)

#define ht_has(ht, str)                                                        \
  ({                                                                           \
    size_t index = ht_hash(ht, str);                                           \
    (ht)->buckets[index] != NULL;                                              \
  })

#define ht_free(ht)                                                            \
  do {                                                                         \
    for (size_t i = 0; i < (ht)->capacity; i++) {                              \
      if ((ht)->buckets[i] != NULL) {                                          \
        free((ht)->buckets[i]->key);                                           \
        free((ht)->buckets[i]);                                                \
      }                                                                        \
    }                                                                          \
    free((ht)->buckets);                                                       \
    (ht)->buckets = NULL;                                                      \
    (ht)->count = 0;                                                           \
    (ht)->capacity = 0;                                                        \
  } while (0)

#define ht_foreach(ht, entry_var)                                              \
  for (size_t _i = 0; _i < (ht)->capacity; ++_i)                               \
    for (Entry *entry_var = (ht)->buckets[_i]; entry_var != NULL;              \
         entry_var = NULL)                                                     \
      if (entry_var)

#ifndef DA_INIT_CAP
#define DA_INIT_CAP 256
#endif // !DA_INIT_CAP

#define DECLTYPE_CAST(T)

#define da_reserve(da, expected_capacity)                                      \
  do {                                                                         \
    if ((expected_capacity) > (da)->capacity) {                                \
      if ((da)->capacity == 0) {                                               \
        (da)->capacity = DA_INIT_CAP;                                          \
      }                                                                        \
      while ((expected_capacity) > (da)->capacity) {                           \
        (da)->capacity *= 2;                                                   \
      }                                                                        \
      (da)->items = DECLTYPE_CAST((da)->items)                                 \
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

typedef struct {
  char *items;
  size_t count;
  size_t capacity;
} StringBuilder;

#define sb_append_null(sb) da_append(sb, 0)

UTILS int sb_appendf(StringBuilder *sb, const char *fmt, ...)
    PRINTF_FORMAT(2, 3);
UTILS int sb_appendf(StringBuilder *sb, const char *fmt, ...) {
  va_list args;

  va_start(args, fmt);
  int n = vsnprintf(NULL, 0, fmt, args);
  va_end(args);

  da_reserve(sb, sb->count + n + 1);
  char *dest = sb->items + sb->count;

  va_start(args, fmt);
  vsnprintf(dest, n + 1, fmt, args);
  va_end(args);

  sb->count += n;

  return n;
}

#endif // !UTILS_IMPLEMENTATION
