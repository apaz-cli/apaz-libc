#include <stdlib.h>

#define MEMDEBUG 1
#include "memdebug.h"

struct LL;
typedef struct LL LL;
struct LL {
    LL* next;
};

int main() {
    size_t num_allocs = 100000;

    LL *ll, *original;
    ll = original = malloc(sizeof(LL));

    for (size_t i = 0; i < num_allocs; i++) {
        ll->next = malloc(sizeof(LL));
        ll = ll->next;
    }

    print_heap();

    for (size_t i = 0; i < num_allocs + 1; i++) {
        ll = original;
        original = original->next;
        free(ll);
    }

    print_heap();
}