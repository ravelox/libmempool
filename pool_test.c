#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libmempool.h"

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

	fprintf(stdout, "===> OVER-ALLOC 2000 bytes (expect NULL)\n");
	void *ptr_fail = pool_alloc(pool, 2000);
	if(ptr_fail)
	{
		fprintf(stdout, "ERROR: over-allocation unexpectedly succeeded\n");
	} else {
		fprintf(stdout, "Over-allocation correctly failed\n");
	}
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

	/* Fragmentation scenario: plenty of total free bytes, no contiguous 150-byte block */
	fprintf(stdout, "===> FRAGMENTATION TEST\n");
	pool = pool_create(1000);
	void *frag1 = pool_alloc(pool, 120);
	void *frag2 = pool_alloc(pool, 120);
	void *frag3 = pool_alloc(pool, 120);
	void *frag4 = pool_alloc(pool, 120);
	void *frag5 = pool_alloc(pool, 120);
	void *frag6 = pool_alloc(pool, 200);
	void *frag7 = pool_alloc(pool, 200); /* consume tail space so no large contiguous free region remains */
	memset(frag1, 'A', 120);
	memset(frag2, 'B', 120);
	memset(frag3, 'C', 120);
	memset(frag4, 'D', 120);
	memset(frag5, 'E', 120);
	memset(frag6, 'F', 200);
	memset(frag7, 'G', 200);
	pool_dump(pool);

	fprintf(stdout, "===> FREE alternating small blocks to fragment\n");
	pool_free(pool, frag1);
	pool_free(pool, frag3);
	pool_free(pool, frag5);
	pool_dump(pool);

	fprintf(stdout, "===> ALLOC 150 bytes (should fail: no contiguous 150-byte block)\n");
	void *frag_fail = pool_alloc(pool, 150);
	if(frag_fail)
	{
		fprintf(stdout, "ERROR: fragmented allocation unexpectedly succeeded\n");
	} else {
		fprintf(stdout, "Fragmented allocation correctly failed despite sufficient total free space\n");
	}
	pool_dump(pool);

	fprintf(stdout, "===> CLEANUP FRAGMENTATION TEST\n");
	pool_free(pool, frag2);
	pool_free(pool, frag4);
	pool_free(pool, frag6);
	pool_free(pool, frag7);
	pool_dump(pool);
	pool_destroy(pool);

	/* Guard enable/disable tests */
	fprintf(stdout, "===> GUARD TEST: ENABLE\n");
	pool = pool_create(256);
	pool_enable_guards(pool, 8);
	void *g1 = pool_alloc(pool, 32);
	memset(g1, 'X', 32);
	pool_dump(pool);
	pool_free(pool, g1);
	pool_dump(pool);

	fprintf(stdout, "===> GUARD TEST: DISABLE (should succeed with no outstanding blocks)\n");
	if(pool_disable_guards(pool))
	{
		fprintf(stdout, "Guards disabled as expected\n");
	} else {
		fprintf(stdout, "ERROR: guards failed to disable with no outstanding blocks\n");
	}
	void *g2 = pool_alloc(pool, 32);
	memset(g2, 'Y', 32);
	pool_dump(pool);

	fprintf(stdout, "===> GUARD TEST: DISABLE WITH OUTSTANDING (should fail)\n");
	void *g3 = pool_alloc(pool, 16);
	memset(g3, 'Z', 16);
	if(pool_disable_guards(pool))
	{
		fprintf(stdout, "ERROR: guards disabled while allocations outstanding\n");
	} else {
		fprintf(stdout, "Guards correctly remained enabled with outstanding blocks\n");
	}
	pool_dump(pool);

	pool_free(pool, g2);
	pool_free(pool, g3);
	fprintf(stdout, "===> GUARD TEST: DISABLE AFTER FREE (should now succeed)\n");
	if(pool_disable_guards(pool))
	{
		fprintf(stdout, "Guards disabled after freeing outstanding blocks\n");
	} else {
		fprintf(stdout, "ERROR: guards still enabled after freeing outstanding blocks\n");
	}
	pool_destroy(pool);
}
