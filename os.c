
#define _GNU_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <sys/mman.h>

#include "os.h"

/* 2^20 pages ought to be enough for anybody */
#define NPAGES	(1024*1024)

static char* pages[NPAGES];

uint64_t alloc_page_frame(void)  
{
	static uint64_t nalloc;
	uint64_t ppn;
	void* va;

	if (nalloc == NPAGES)
		errx(1, "out of physical memory");

	/* OS memory management isn't really this simple */
	ppn = nalloc;
	nalloc++;

	va = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (va == MAP_FAILED)
		err(1, "mmap failed");

	pages[ppn] = va;	
	return ppn;
}

void* phys_to_virt(uint64_t phys_addr)
{
	uint64_t ppn = phys_addr >> 12;
	uint64_t off = phys_addr & 0xfff;
	char* va = NULL;

	if (ppn < NPAGES)
		va = pages[ppn] + off;

	return va;
}

int main(int argc, char **argv)
{
	uint64_t pt = alloc_page_frame();
	uint64_t new_pt = alloc_page_frame();
	uint64_t *tmp;
	
	/* 1st Test */
	assert(page_table_query(pt, 0xcafe) == NO_MAPPING);
	page_table_update(pt, 0xcafe, 0xf00d);
	page_table_update(pt, 0xbaff, 0xbadd);
	assert(page_table_query(pt, 0xcafe) == 0xf00d);
	assert(page_table_query(pt, 0xbaff) == 0xbadd);
	page_table_update(pt, 0xcafe, NO_MAPPING);
	assert(page_table_query(pt, 0xcafe) == NO_MAPPING);
	assert(page_table_query(pt, 0xbaff) == 0xbadd);
	page_table_update(pt, 0xbaff, NO_MAPPING);
	assert(page_table_query(pt, 0xbaff) == NO_MAPPING);
	printf("1st Test: PASSED\n");
	
	/* 2nd Test*/
	page_table_update(pt, 0xcafe, 0);
	assert(page_table_query(pt, 0xcafe) == 0);
	page_table_update(pt, 0xcafe, NO_MAPPING);
	assert(page_table_query(pt, 0xcafe) == NO_MAPPING);
	printf("2nd Test: PASSED\n");
	
	/* 3rd Test */
	page_table_update(pt, 0x8686, 0xabcd);
	assert(page_table_query(pt, 0x8686) == 0xabcd);
	page_table_update(pt, 0x8686, 0x1234);
	assert(page_table_query(pt, 0x8686) == 0x1234);
	printf("3rd Test: PASSED\n");
	
	/* 4th Test */
	page_table_update(pt, 0xcafe, 0xacdc);
	assert(page_table_query(pt, 0xcafe) == 0xacdc);
	page_table_update(pt, 0xcaff, 0xaaaa);
	assert(page_table_query(pt, 0xcafe) == 0xacdc);
	page_table_update(pt, 0xcafe, NO_MAPPING);
	page_table_update(pt, 0xcaff, NO_MAPPING);
	assert(page_table_query(pt, 0xcafe) == NO_MAPPING);
	assert(page_table_query(pt, 0xcaff) == NO_MAPPING);
	printf("4th Test: PASSED\n");
	
	/* 5th Test */
	page_table_update(new_pt, 0xcafe, 0xabcd);
	assert(page_table_query(new_pt, 0xcafe) == 0xabcd);
	page_table_update(new_pt, 0xcafe, NO_MAPPING);
	assert(page_table_query(new_pt, 0xcafe) == NO_MAPPING);
	printf("5th Test: PASSED\n");
	
	/* 6th Test */
	page_table_update(pt, 0x1ffff8000000, 0x1212);
	assert(page_table_query(pt, 0x1ffff8000000) == 0x1212);
	tmp = phys_to_virt(pt << 12);
	tmp = phys_to_virt((tmp[511] >> 12) << 12);
	tmp = phys_to_virt((tmp[511] >> 12) << 12);
	tmp = phys_to_virt((tmp[0] >> 12) << 12);
	tmp = phys_to_virt((tmp[0] >> 12) << 12);
	tmp[0] = ((tmp[0] >> 1) << 1);
	assert(page_table_query(pt, 0x1ffff8000000) == NO_MAPPING);
	printf("6th Test: PASSED\n\n----------------\n");
	
	printf("Overall:  PASSED\n\n");
	
	
	
	return 0;
}
