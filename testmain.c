/**********************************************************************
 *
 *  SLAB USAGE 
 *
 *
 *  HOW TO BUILD:
 *
 *   - unix:     gcc testmain.c -o testmain
 *   - windows:  cl testmain.c   
 *
 **********************************************************************/


#include "imembase.c"
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


