#include <stdio.h>
#include <stdlib.h>

#include "mempool.h"

int
main(int argc, char *argv[])
{
	MemoryPool *pool;
	void *ptr1, *ptr2, *ptr3, *ptr4, *ptr5;

	fprintf(stdout, "===> CREATING POOL\n");
	pool = pool_create(1000);
	pool_dump(pool);


	fprintf(stdout, "===> ALLOC 100 bytes POOL\n");
	ptr1 = pool_alloc(pool, 100);
	memset(ptr1, '1', 100);
	pool_dump(pool);

	fprintf(stdout, "===> ALLOC 200 bytes \n");
	ptr2 = pool_alloc(pool, 200);
	memset(ptr2, '2', 200);
	pool_dump(pool);
	
	fprintf(stdout, "===> ALLOC 300 bytes \n");
	ptr3 = pool_alloc(pool, 300);
	memset(ptr3, '3', 300);
	pool_dump(pool);

	fprintf(stdout, "===> ALLOC 400 bytes \n");
	ptr4 = pool_alloc(pool, 400);
	memset(ptr4, '4', 400);
	pool_dump(pool);

	fprintf(stdout, "===> FREE PTR4 400 bytes \n");
	pool_free(pool, ptr4);
	pool_dump(pool);

	fprintf(stdout, "===> FREE PTR2 200 bytes \n");
	pool_free(pool, ptr2);
	pool_dump(pool);

	fprintf(stdout, "===> ALLOC 50 bytes \n");
	ptr5 = pool_alloc(pool, 50);
	memset(ptr5, '5', 50);
	pool_dump(pool);

	fprintf(stdout, "===> FREE PTR3 300 bytes \n");
	pool_free(pool, ptr3);
	pool_dump(pool);
	
	fprintf(stdout, "===> FREE PTR5 50 bytes \n");
	pool_free(pool, ptr5);
	pool_dump(pool);
	
	fprintf(stdout, "===> FREE PTR1 100 bytes \n");
	pool_free(pool, ptr1);
	pool_dump(pool);

	fprintf(stdout, "===> DESTROY POOL\n");
	pool_destroy(pool);
}
