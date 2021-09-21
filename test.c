
#define MEMDEBUG 1
#define PRINT_MEMALLOCS 1

#include <apaz-libc.h>

int main() {
  Arena *arena = Arena_new("TestArena");
  int *arr = Arena_malloc_n_of(arena, 30, int);
  Arena_pop_n_of(arena, 15, int);
  Arena_pop_n_of(arena, 15, int);
  Arena_pop_n_of(arena, 500, bool);
}