memslab
=======

Slab Memory Allocator in Application Layer
This is an enhanced SLAB algorithm implementation in application layer,
which provides O(1) memory allocating and efficient memory recycling.

Since SUNOS has presented slab allocation theroy, many OSs have implemented
slab in their kernel. But it requires kernel-layer interfaces such as page
supply etc. So this library improves slab's algorithm and brings the
interfaces of slab into application layer:

- application layer slab allocator implementation
- O(1) allocating / free: almost speed up 500% - 1200% vs malloc
- re-implementation of page supplier: with new "SLAB-Tree" algorithm
- memory recycle: automatic give memory back to os to avoid wasting
- 30% - 50% memory wasting
- platform independence


Example
=======
```cpp
#include "imembase.h"
#include <stdio.h>

int main(void)
{
char *ptr;

/* init kmem interface */
ikmem_init(0, 0, 0);

ptr = (char*)ikmem_malloc(8);
assert(ptr);

printf("sizeof(ptr)=%d\n", ikmem_ptr_size(ptr));

ptr = ikmem_realloc(ptr, 40);
assert(ptr);

printf("sizeof(ptr)=%d\n", ikmem_ptr_size(ptr));

ikmem_free(ptr);

/* clean environment */
ikmem_destroy();

return 0;
}
```

