#ifndef INCLUDE_LISTUTIL
#define INCLUDE_LISTUTIL
#include <stdbool.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define LIST_DEFINE(type)                                                      \
  typedef type *List_##type;                                                   \
                                                                               \
  /* Builtin Utility Functions */                                              \
                                                                               \
  static inline void __List_##type##_setlen(List_##type list,                  \
                                            size_t new_len) {                  \
    *(((size_t *)list) - 2) = new_len;                                         \
  }                                                                            \
  static inline void __List_##type##_setcap(List_##type list,                  \
                                            size_t new_cap) {                  \
    *(((size_t *)list) - 1) = new_cap;                                         \
  }                                                                            \
                                                                               \
  /**                                                                          \
   * Constructs a new list of the specified capacity. The returned list must   \
   * be freed with List_##type##_destroy(), as it is allocated in a clever way \
   * by malloc(). Do not call free() on any List_##type.                       \
   *                                                                           \
   * The list returned has current size 0.                                     \
   */                                                                          \
  static inline List_##type List_##type##_new_cap(size_t capacity) {           \
    void *vptr = malloc(sizeof(size_t) * 2 + sizeof(type) * capacity);         \
    size_t *stptr = (size_t *)vptr;                                            \
    *stptr = 0;                                                                \
    stptr += 1;                                                                \
    *stptr = capacity;                                                         \
    stptr += 1;                                                                \
    return (List_##type)stptr;                                                 \
  }                                                                            \
                                                                               \
  /**                                                                          \
   * Constructs a new list of the specified capacity. The returned list must   \
   * be freed with List_##type##_destroy(), as it is allocated in a clever way \
   * by malloc(). Do not call free() on any List_##type.                       \
   *                                                                           \
   * The list returned has current size len.                                   \
   */                                                                          \
  static inline List_##type List_##type##_new_len(size_t len) {                \
    void *vptr = malloc(sizeof(size_t) * 2 + sizeof(type) * len);              \
    size_t *stptr = (size_t *)vptr;                                            \
    *stptr = len;                                                              \
    stptr += 1;                                                                \
    *stptr = len;                                                              \
    stptr += 1;                                                                \
    return (List_##type)stptr;                                                 \
  }                                                                            \
                                                                               \
  /**                                                                          \
   * Constructs a new list of the specified capacity, using num_items many     \
   * members from the passed buffer.                                           \
   *                                                                           \
   * The list returned has current size num_items.                             \
   */                                                                          \
  static inline List_##type List_##type##_new_of(                              \
      type *items, size_t num_items, size_t capacity) {                        \
    List_##type nl = List_##type##_new_cap(capacity);                          \
    for (size_t i = 0; i < num_items; i++)                                     \
      nl[i] = items[i];                                                        \
    __List_##type##_setlen(nl, num_items);                                     \
    return nl;                                                                 \
  }                                                                            \
                                                                               \
  /**                                                                          \
   * This function realloc()s the list. The new list retains its length if the \
   * new capacity is greater than or equal to the old length, but items are    \
   * trimmed from the end if the new capacity is less than the old length.     \
   *                                                                           \
   * Since realloc() is called explicitly, the old list pointer is             \
   * invalidated.                                                              \
   */                                                                          \
  static inline List_##type List_##type##_resize(List_##type to_resize,        \
                                                 size_t new_capacity) {        \
    size_t *vptr = ((size_t *)to_resize) - 2;                                  \
    const size_t new_s = sizeof(size_t) * 2 + sizeof(type) * new_capacity;     \
    size_t *stptr = (size_t *)realloc(vptr, new_s);                            \
    *stptr = MIN(new_capacity, *((size_t *)stptr));                            \
    stptr += 1;                                                                \
    *stptr = new_capacity;                                                     \
    stptr += 1;                                                                \
    return (List_##type)stptr;                                                 \
  }                                                                            \
  static inline void List_##type##_destroy(List_##type to_destroy) {           \
    free(((size_t *)to_destroy) - 2);                                          \
  }                                                                            \
  static inline size_t List_##type##_len(List_##type list) {                   \
    return *(((size_t *)list) - 2);                                            \
  }                                                                            \
  static inline size_t List_##type##_cap(List_##type list) {                   \
    return *(((size_t *)list) - 1);                                            \
  }                                                                            \
  static inline void List_##type##_trim(List_##type list) {                    \
    List_##type##_resize(list, List_##type##_len(list));                       \
  }                                                                            \
  static inline List_##type List_##type##_clone(List_##type to_clone) {        \
    size_t new_len = List_##type##_len(to_clone);                              \
    List_##type nl = List_##type##_new_len(new_len);                           \
    for (size_t i = 0; i < new_len; i++)                                       \
      nl[i] = to_clone[i];                                                     \
    return nl;                                                                 \
  }                                                                            \
  static inline void List_##type##_add(List_##type *list_ref,                  \
                                       type to_append) {                       \
    List_##type list = *list_ref;                                              \
    size_t current_len = List_##type##_len(list);                              \
    size_t current_cap = List_##type##_cap(list);                              \
    __List_##type##_setlen(list, current_len + 1);                             \
    if (current_len == current_cap)                                            \
      *list_ref = list =                                                       \
          List_##type##_resize(list, (size_t)(current_cap * 1.5) + 16);        \
    list[current_len] = to_append;                                             \
  }                                                                            \
  static inline List_##type List_##type##_addeq(List_##type list,              \
                                                type to_append) {              \
    size_t current_len = List_##type##_len(list);                              \
    size_t current_cap = List_##type##_cap(list);                              \
    __List_##type##_setlen(list, current_len + 1);                             \
    if (current_len == current_cap)                                            \
      list = List_##type##_resize(list, (size_t)(current_cap * 1.5) + 16);     \
    list[current_len] = to_append;                                             \
    return list;                                                               \
  }                                                                            \
  static inline void List_##type##_pop(List_##type list) {                     \
    __List_##type##_setlen(list, List_##type##_len(list) - 1);                 \
  }                                                                            \
  static inline type *List_##type##_peek(List_##type list) {                   \
    return list + (List_##type##_len(list) - 1);                               \
  }                                                                            \
  static inline List_##type List_##type##_sublist(List_##type list,            \
                                                  size_t from, size_t to) {    \
    /* TODO Figure out asserts. */                                             \
    return list;                                                               \
  }                                                                            \
                                                                               \
  /* For fear of redefinition below, define filter and foreach here. */        \
                                                                               \
  static inline List_##type List_##type##_filter(                              \
      List_##type list, bool (*filter_fn)(type, void *), void *extra_data) {   \
    /* Since the list would be destroyed anyway, it can be reused. */          \
    size_t len = List_##type##_len(list);                                      \
    for (size_t i = 0, retained = 0; i < len; i++)                             \
      if (filter_fn(list[i], extra_data))                                      \
        list[retained++] = list[i];                                            \
    return list;                                                               \
  }                                                                            \
                                                                               \
  static inline void List_##type##_foreach(                                    \
      List_##type list, void (*action_fn)(type, void *), void *extra_data) {   \
    size_t len = List_##type##_len(list);                                      \
    for (size_t i = 0; i < len; i++)                                           \
      action_fn(list[i], extra_data);                                          \
    List_##type##_destroy(list);                                               \
  }

// TODO figure out how to get zip() working.
// TODO cross product
// TODO sort

#define LIST_DEFINE_MONAD(type, map_type)                                      \
  static inline List_##map_type List_##type##_map_to_##map_type(               \
      List_##type list, map_type (*mapper)(type, void *), void *extra_data) {  \
    List_##map_type nl = List_##map_type##_new_len(List_##type##_len(list));   \
    size_t len = List_##type##_len(list);                                      \
    for (size_t i = 0; i < len; i++)                                           \
      nl[i] = mapper(list[i], extra_data);                                     \
    List_##type##_destroy(list);                                               \
    return nl;                                                                 \
  }                                                                            \
  static inline List_##map_type List_##type##_flatmap_to_##map_type(           \
      List_##type list, List_##map_type (*mapper)(type, void *),               \
      void *extra_data) {                                                      \
    size_t nll_len = List_##type##_len(list), size_sum = 0;                    \
    List_List_##map_type nll = List_List_##map_type##_new_len(nll_len);        \
    for (size_t i = 0; i < nll_len; i++) {                                     \
      List_##map_type ml = mapper(list[i], extra_data);                        \
      nll_len += List_##map_type##_len(ml);                                    \
      nll[i] = ml;                                                             \
    }                                                                          \
    /* TODO Optimize with buffer re-use if possible.*/                         \
    /* TODO Debate using a VLA in place of nll. */                             \
    List_##map_type retlist = List_##map_type##_new_len(size_sum);             \
    for (size_t i = 0; i < nll_len; i++) {                                     \
      size_t sublen = List_##map_type##_len(nll[i]);                           \
      for (size_t j = 0; j < sublen; j++) {                                    \
        retlist[j] = nll[i][j];                                                \
      }                                                                        \
      List_destroy(nll[i]);                                                    \
    }                                                                          \
    List_##type##_destroy(list);                                               \
    return retlist;                                                            \
  }

#endif // INCLUDE_LISTUTIL