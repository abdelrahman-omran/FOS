/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
__inline__ uint32 get_block_size(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (*curBlkMetaData) & ~(0x1);
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
__inline__ int8 is_free_block(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (~(*curBlkMetaData) & 0x1) ;
}

//===========================
// 3) ALLOCATE BLOCK:
//===========================

void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockElement* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", get_block_size(blk), is_free_block(blk)) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

bool is_initialized = 0;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//==================================================================================
	// DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (initSizeOfAllocatedSpace % 2 != 0)
			initSizeOfAllocatedSpace++; // ensure it's multiple of 2
		if (initSizeOfAllocatedSpace == 0)
			return;
		is_initialized = 1;
	}
	//==================================================================================
	//==================================================================================

	uint32 *stAdress = (uint32 *)daStart;
	uint32 *edAdress = (uint32 *)(daStart + initSizeOfAllocatedSpace - sizeof(uint32));

	*stAdress =  1;
	*edAdress = 1;

	uint32 totalSize = initSizeOfAllocatedSpace - 2 * sizeof(uint32);

	uint32 *first_free_block = (uint32 *)(stAdress + 1);
	*first_free_block = totalSize ;

	uint32 *blkFooter = (uint32 *)((uint32)first_free_block + totalSize - sizeof(uint32));
	*blkFooter = totalSize ;

	LIST_INIT(&freeBlocksList);
	struct BlockElement *freeBlock = (struct BlockElement *)(first_free_block + 1);
	LIST_INSERT_HEAD(&freeBlocksList, freeBlock);
}
//==================================
// [2] SET BLOCK HEADER & FOOTER:
//==================================
void set_block_data(void *va, uint32 totalSize, bool isAllocated)
{
	uint32 *hptr = (uint32 *)va-1;

	*hptr = totalSize ;
	if(isAllocated) *hptr|=1;

	uint32 tail = (uint32)va + totalSize - 2*(sizeof(uint32));
	uint32 *tptr = (uint32 *)tail;

	*tptr = totalSize ;
	if(isAllocated)*tptr|=1;
}


//=========================================
// [3] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{

		//==================================================================================
	// DON'T CHANGE THESE LINES==========================================================
	//==================================================================================

	if (size % 2 != 0)
		size++; // ensure that the size is even (to use LSB as allocation flag)
	if (size < DYN_ALLOC_MIN_BLOCK_SIZE)

		size = DYN_ALLOC_MIN_BLOCK_SIZE;
	if (!is_initialized)
	{
		uint32 required_size = size + 2 * sizeof(int) + 2 * sizeof(int);
		uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE) / PAGE_SIZE);
		uint32 da_break = (uint32)sbrk(0);
		initialize_dynamic_allocator(da_start, da_break - da_start);
	}

	//==================================================================================
	//==================================================================================

	if (size == 0) return NULL;

		uint32 effectiveSize = size + 8;
		struct BlockElement *blk;
		LIST_FOREACH(blk, &freeBlocksList)
		{

			uint32 blkSize = get_block_size(blk);
			if (blkSize >= effectiveSize){


				if(blkSize - effectiveSize >= 16){
                    struct BlockElement *newFreeBlock = (struct BlockElement *)((char *)blk + effectiveSize );
                    uint32 newBlockSize = blkSize - effectiveSize;
                    set_block_data(newFreeBlock, newBlockSize, 0);
                    LIST_INSERT_AFTER(&freeBlocksList, blk, newFreeBlock);
				}
				else{
					effectiveSize = blkSize;
				}
                    set_block_data(blk,effectiveSize,1);
                    LIST_REMOVE(&freeBlocksList, blk);
                    return (void *)((char *)blk  );

			}
		}

        void *newBlock = sbrk(effectiveSize);
        if (get_block_size(newBlock)<effectiveSize) return NULL;
		else return (void *)((char *)newBlock);

}
//=========================================
// [4] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
		//==================================================================================
	// DON'T CHANGE THESE LINES==========================================================
	//==================================================================================

	if (size % 2 != 0)
		size++; // ensure that the size is even (to use LSB as allocation flag)
	if (size < DYN_ALLOC_MIN_BLOCK_SIZE)

		size = DYN_ALLOC_MIN_BLOCK_SIZE;
	if (!is_initialized)
	{
		uint32 required_size = size + 2 * sizeof(int) + 2 * sizeof(int);
		uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE) / PAGE_SIZE);
		uint32 da_break = (uint32)sbrk(0);
		initialize_dynamic_allocator(da_start, da_break - da_start);
	}

	//==================================================================================
	//==================================================================================
	if(1==1){
switch (size) {
    case 0:
        return NULL;

}
}

	uint32 anaA5rSize = size + 8; 
	struct BlockElement *eslamItrator;
	struct BlockElement *ansabBlock = NULL;
	int el7ala = 0;
	if (anaA5rSize < 16)
		anaA5rSize = 16;
	int x = 0;
	LIST_FOREACH(eslamItrator, &freeBlocksList)
	{
		uint32 ana7agmElBlock = get_block_size(eslamItrator); 

int karar = 0;
switch (1) {
    case 1:
if (ana7agmElBlock == anaA5rSize) {
    karar = 1; 
} else if (ana7agmElBlock >= anaA5rSize && ana7agmElBlock >= 16 && ansabBlock == NULL) {
    karar = 2; 
} else if (ana7agmElBlock >= anaA5rSize && ana7agmElBlock >= 16) {
    if (ana7agmElBlock < get_block_size(ansabBlock)) {
        karar = 3; 
    }
}
}
switch (karar) {
    case 1: 
        set_block_data(eslamItrator, get_block_size(eslamItrator), 1);
        LIST_REMOVE(&freeBlocksList, eslamItrator);
        return (void *)((char *)eslamItrator);

    case 2: 
	        ansabBlock = eslamItrator;
        if (ana7agmElBlock - anaA5rSize >= 16) {
            el7ala = 1;
        } else {
            el7ala = 2;
        }
        x++;
        continue;

    case 3: 
        if (ana7agmElBlock < get_block_size(ansabBlock)) {
            if (ana7agmElBlock - anaA5rSize >= 16) {
                ansabBlock = eslamItrator;
                el7ala = 1; 
            } else {
                ansabBlock = eslamItrator;
                el7ala = 2;
            }
        }
        break;

}
if(1 != 1)
		x++;
	}

	switch (el7ala) {
    case 0:
        if (ansabBlock == NULL) {
            void *testGded = sbrk(anaA5rSize);
            if (testGded == (void *)-1) {
                return NULL;
            }
        }
        break; 

    case 1: {
        struct BlockElement *new_test = (struct BlockElement *)((char *)ansabBlock + anaA5rSize);
        uint32 new_test_size = get_block_size(ansabBlock) - anaA5rSize;
        set_block_data(new_test, new_test_size, 0);

        LIST_INSERT_AFTER(&freeBlocksList, ansabBlock, new_test);
        set_block_data(ansabBlock, anaA5rSize, 1); 
        LIST_REMOVE(&freeBlocksList, ansabBlock);    

        return (void *)((char *)ansabBlock);
    }

    case 2:
        set_block_data(ansabBlock, get_block_size(ansabBlock), 1);
        LIST_REMOVE(&freeBlocksList, ansabBlock);
        return (void *)((char *)ansabBlock);

   
}

	
	return NULL;
}

//===================================================
// [5] FREE BLOCK WITH COALESCING:
//===================================================
void  free_block(void *va) {
    if (va == NULL) {
        return; // Cannot free a NULL pointer
    }

	if(is_free_block(va)) return;

	uint32 blkSize = get_block_size(va);
	set_block_data(va , blkSize , 0) ;


	struct BlockElement *blk ;
	struct BlockElement *v = va ;

	uint32 listSize = LIST_SIZE(&freeBlocksList) ;
	if(listSize == 0 || v<LIST_FIRST(&freeBlocksList)){
		LIST_INSERT_HEAD(&freeBlocksList , v) ;
	}
	else if(v>LIST_LAST(&freeBlocksList)){
		LIST_INSERT_TAIL(&freeBlocksList,v);
	}
	else{

		LIST_FOREACH(blk,&freeBlocksList){
			if(v>blk && v<LIST_NEXT(blk)){
				LIST_INSERT_AFTER(&freeBlocksList,blk,v);
			}
		}
	}


	// merge with next
	uint32 *curBlkMetaData1 = ((uint32 *)va + blkSize -1) ;
    uint32 freeNxt = is_free_block((void*)va +blkSize) ;
    uint32 nxtSize =  get_block_size((void*)va +blkSize);

    if(freeNxt){
	struct BlockElement *nxtBlock = LIST_NEXT(v);
	LIST_REMOVE(&freeBlocksList,nxtBlock );
	blkSize+=nxtSize;
    set_block_data(v , blkSize , 0) ;
    }

	// merge with prev

	uint32 *curBlkMetaData = ((uint32 *)va - 2) ;
	uint32 freePrev= (~(*curBlkMetaData) & 0x1);
	uint32 prevSize =  (*curBlkMetaData) & ~(0x1);

	if(freePrev){
	struct BlockElement *newBlock = LIST_PREV(v);
	LIST_REMOVE(&freeBlocksList,v);
	set_block_data(newBlock , prevSize+blkSize , 0) ;
	}

}
//=========================================
// [6] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void *va, uint32 new_size) {
    // TODO: [PROJECT'24.MS1 - #08] [3] DYNAMIC ALLOCATOR - realloc_block_FF
    // COMMENT THE FOLLOWING LINE BEFORE START CODING
    // panic("realloc_block_FF is not implemented yet");
    // Your Code is Here...

    // -- Handle edge cases --
    if (va == NULL) {
        if (new_size != 0) {
            return alloc_block_FF(new_size);
        }
        return NULL;
    }
    if (new_size == 0) {
        free_block(va);
        return NULL;
    }
    // -----------------------
    uint32 old_size = get_block_size(va);
    new_size+=8;
    if (new_size > old_size) {
        uint32 next_block_size = get_block_size((void*)((uint32)va + old_size));
        // if next block is free and can accommodate new size -> merge
        cprintf("Because: %d + %d >= %d \n", next_block_size, old_size, new_size);
        if (is_free_block((uint32 *)va + old_size) && (next_block_size + old_size >= new_size)) {
        	cprintf("no need to relocate\n");
            uint32 rem_size = next_block_size + old_size - new_size;
            if (rem_size < 16) {
                set_block_data(va, next_block_size + old_size, 1);
                LIST_REMOVE(&freeBlocksList,(struct BlockElement*)((uint32)va + old_size));
            } else {
            	// remove old free block from list
            	set_block_data((void*)((uint32)va + old_size), next_block_size, 1);
            	LIST_REMOVE(&freeBlocksList,(struct BlockElement*)((uint32)va + old_size));
            	//
                set_block_data(va, new_size, 1);
                set_block_data((void*)((uint32)va + new_size), rem_size, 1);
                free_block((void*)((uint32)va + new_size));
            }
            return va;
        } else {
            // need to move the block to a bigger one
        	cprintf("relocate to a bigger block\n");
            void *new_block = alloc_block_FF(new_size);
            if (new_block == NULL)
                return NULL;
            //
            free_block(va);
            return new_block;
        }
    } else if (new_size < old_size) {
    	if(new_size < 16){
    		return NULL;
    	}
        // is next block free? -> coalesce
        if (is_free_block((void*)((uint32)va + old_size))) {
            // resize old block
            set_block_data(va, new_size, 1);
            // set extra space to a free block
            set_block_data((void*)((uint32)va + new_size), (old_size - new_size), 1);
            // free new block and add to free list
            free_block((void*)((uint32)va + new_size));
        }
        else {
        	cprintf("NO coalesce, address is: %x \n", va);
            // if remaining diff can't create a new block ->
        	// no resize | search for a smaller free block (TODO check if needed)
            if ((old_size - new_size) < 16) {
            	1;
            }
            // else: separate block, and free extra space
            else {
                // resize old block
                set_block_data(va, new_size, 1);
                // set extra space to a free block
                set_block_data((void*)((uint32)va + new_size), (old_size - new_size), 1);
                // free new block and add to free list
                free_block((void*)((uint32)va + new_size));
            }
            return va;
        }
    }
    else {
    	// new size == old size
        return va;
    }

    return NULL;
}


/*********************************************************************************************/
/*********************************************************************************************/
/*********************************************************************************************/
//=========================================
// [7] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [8] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}
