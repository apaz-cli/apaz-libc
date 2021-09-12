
#include <apaz-libc.h>

static inline size_t factorial(size_t x, void *extra_data) {
  return x ? x * factorial(x - 1, extra_data) : 1;
}

static inline void print_size_t(size_t to_print, void *extra_data) {
  printf("%zu\n", to_print);
}

// This macro declares all the functionality of a list of the given type.
LIST_DEFINE(size_t);
// You can easily define lists of lists.
LIST_DEFINE(List_size_t);
// This is the fun part. (original type, map to type)
LIST_DEFINE_MONAD(size_t, size_t);

int main() {
  // The type of the list is List_##type, as provided to LIST_DEFINE. There
  // are a bunch of ways to define a list, they all have _new in their name.
  List_size_t list = List_size_t_new_len(10);

  // It's just a size_t*. You can use it as such.
  for (size_t i = 0; i < List_size_t_len(list); i++)
    list[i] = i;

  // Add new elements whenever you want, and it will grow to meet demand.
  // There are two syntaxes for this, whichever you prefer.
  List_size_t_add(&list, 11);
  list = List_size_t_addeq(list, 12);

  // You can also peek and pop.
  size_t peeked = *List_size_t_peek(list);
  List_size_t_pop(list);

  // You can clone lists too.
  // Lists are heap allocated, so destroy them when you're done with them.
  // Don't try to use normal free() through. Use List_##type##_destroy() instead.
  List_size_t cloned = List_size_t_clone(list);
  List_size_t_destroy(list);

  // Lists also support operations like map(), filter(), flatmap(), and
  // foreach().
  // These operations destroy the list passed to them, and if they return a new
  // list they re-use the space allocated for the old one where possible.
  cloned = List_size_t_map_to_size_t(cloned, factorial, NULL);
  List_size_t_foreach(cloned, print_size_t, NULL);
}