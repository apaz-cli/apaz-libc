#define MEMDEBUG 1
#define PRINT_MEMALLOCS 1
#include <apaz-libc.h>

/* The string module can make use of the list and memdebug modules. */
void String_printAndDestroy(String str) {
  String_println(str);
  String_destroy(str);
}

int main() {
  /* Basic usage looks great. Just do string stuff. */
  String s = String_new_of_strlen("I can make Strings");
  String_println(s);
  s = String_toUpper(s);
  assert(String_endsWith(s, "STRINGS"));
  String_println(s);

  /* There's also all the convenience methods you're used to, like split(). */
  List_String l = String_split(s, " ");
  String_destroy(s);

  l = List_String_map_to_String(l, String_toLower);

  /* We can still get its length, add items, and address it like we would a
   * fixed size array. However, this is all very tedious of course. Also, we
   * need to pay very careful attention that we free all the memory. */
  l = List_String_addeq(l, String_new_of_strlen("!"));

  /* Since it returns a list.h list, we can use monads.
    We can destroy all the strings in the list inside the monad. */
  List_String_foreach(l, String_printAndDestroy);

  print_heap();
  puts("All memory cleaned up.");
}