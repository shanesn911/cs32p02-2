/*
 * mm-explicit.c - The best malloc package EVAR, now with an explicit free list!
 *
 * This is the only file you edit and submit. It is the Part 1 starter code,
 * updated for Part 2: a new block_t, a free list head, and stubs for the new
 * helper functions. Every function below has a comment describing what it
 * does.
 *
 * Setup (before Checkpoint 1): bring in your Part 1 work.
 *   - Paste the BODIES of your Part 1 mm_malloc, mm_realloc, and mm_calloc
 *     into the matching functions below, replacing their bodies. Look for
 *     the "PASTE" comments.
 *   - In mm_malloc, leave out your call to coalesce_free_blocks.
 *   - If you wrote extra helper functions in Part 1 (other than
 *     coalesce_free_blocks), paste them where marked below the helpers.
 *   - Do NOT paste your old block_t, mm_heap_first, or mm_heap_last: they
 *     are already here.
 *   - With the new block_t, sizeof(block_t) is 24, not 8. Wherever your
 *     pasted code uses sizeof(block_t) to mean "the size of the header",
 *     use sizeof(size_t) instead.
 *
 * Then work through the checkpoints in the writeup in order:
 *
 *   Checkpoint 1: the explicit free list (list_add, list_remove)
 *   Checkpoint 2: footers (get_footer, set_footer)
 *   Checkpoint 3: immediate coalescing (get_prev_block)
 *
 * A note on C: this file is C, not C++. Two things in it are legal here
 * but not in C++:
 *   - Adding an integer to a `void *` moves it that many BYTES. So
 *     `(void *) block + 32` is the address 32 bytes after `block`, no
 *     matter what type `block` points to.
 *   - A `void *` converts to any other pointer type without a cast, so
 *     `block_t *b = mem_sbrk(32);` compiles.
 */

#include <stdint.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/** The required alignment of heap payloads */
const size_t ALIGNMENT = 2 * sizeof(size_t);
const size_t MIN_BLOCK_SIZE = 4 * sizeof(size_t);

/**
 * The layout of each block on the heap.
 *
 * The typedef comes first so that `next` and `prev` can use the name
 * block_t inside the struct's own definition.
 *
 * The union stores its members in the SAME bytes, starting right after the
 * header: a free block uses them for its free list pointers, and an
 * allocated block hands them to the user as payload. A block is never both
 * at once.
 *
 * sizeof(block_t) is 24: the 8-byte header plus the union, which is as big
 * as its largest member (two 8-byte pointers). The payload array takes no
 * space; offsetof(block_t, payload) is still 8.
 *
 * payload is declared [0], not [] as in Part 1: C does not allow a
 * flexible array member (`[]`) inside a union.
 */
typedef struct block_t block_t;

struct block_t {
    /** The size of the block and whether it is allocated (stored in the low bit) */
    size_t header;
    union {
        /** For FREE blocks: the next and previous blocks on the free list */
        struct {
            block_t *next;
            block_t *prev;
        };
        /** For ALLOCATED blocks: the start of the user's data */
        uint8_t payload[0];
    };
};

/**
 * The first and last blocks on the heap. Both are NULL when the heap has
 * no blocks yet. Any code that adds, splits, or merges blocks must keep
 * mm_heap_last pointing at the block with the highest address.
 */
static block_t *mm_heap_first = NULL;
static block_t *mm_heap_last = NULL;

/** The first block on the free list, or NULL when no block is free */
static block_t *free_list_head = NULL;

/** Rounds up `size` to the nearest multiple of `n` */
static size_t round_up(size_t size, size_t n) {
    return (size + (n - 1)) / n * n;
}

/** Extracts a block's size from its header */
static size_t get_size(block_t *block) {
    return block->header & ~1;
}

/** Extracts a block's allocation state from its header */
static bool is_allocated(block_t *block) {
    return block->header & 1;
}

/*
 * Some helpers below are new in Part 2. __attribute__((unused)) stops the
 * compiler from warning that a function is never called. You can remove it
 * once you call the function.
 */

/** get_footer - Returns a pointer to the block's footer: its last 8 bytes. (Checkpoint 2) */
__attribute__((unused))
static size_t *get_footer(block_t *block) {
    (void) block;
    // TODO (Checkpoint 2)
    return NULL;
}

/** set_footer - Copies the block's header into its footer. (Checkpoint 2) */
__attribute__((unused))
static void set_footer(block_t *block) {
    (void) block;
    // TODO (Checkpoint 2)
}

/**
 * Set's a block's header with the given size and allocation state.
 * `size` is the size of the whole block and must be a multiple of
 * ALIGNMENT, so its low bit is free to hold `is_allocated`. (Checkpoint 2)
 */
static void set_header(block_t *block, size_t size, bool is_allocated) {
    block->header = size | is_allocated;
}

/**
 * Finds the first free block with at least the given size.
 * `size` is a whole-block size. If no block is large enough, returns NULL.
 * (Checkpoint 1)
 */
static block_t *find_fit(size_t size) {
    // Traverse the blocks in the heap using the implicit list
    for (
        block_t *curr = mm_heap_first;
        mm_heap_last != NULL && curr <= mm_heap_last;
        curr = (void *) curr + get_size(curr)
    ) {
        // If the block is free and large enough for the allocation, return it
        if (!is_allocated(curr) && get_size(curr) >= size) {
            return curr;
        }
    }
    return NULL;
}

/**
 * Gets the header corresponding to a given payload pointer.
 * offsetof(block_t, payload) is the distance in bytes from the start of a
 * block_t to its payload field, which is 8 (the header).
 */
static block_t *block_from_payload(void *ptr) {
    return ptr - offsetof(block_t, payload);
}

/** list_add - Adds a free block to the front of the free list. (Checkpoint 1) */
__attribute__((unused))
static void list_add(block_t *block) {
    (void) block;
    // TODO (Checkpoint 1)
}

/** list_remove - Removes a block from the free list. (Checkpoint 1) */
__attribute__((unused))
static void list_remove(block_t *block) {
    (void) block;
    // TODO (Checkpoint 1)
}

/**
 * get_prev_block - Returns a pointer to the header of the block just before
 *      `block` in memory, found by reading that block's footer. Returns NULL
 *      if `block` is the first block on the heap. (Checkpoint 3)
 */
__attribute__((unused))
static block_t *get_prev_block(block_t *block) {
    (void) block;
    // TODO (Checkpoint 3)
    return NULL;
}

/*
 * PASTE: your own Part 1 helper functions (if any) go here.
 */


/**
 * mm_init - Initializes the allocator state
 *
 * The driver calls mm_init before every trace, on a fresh, empty heap.
 * Reset ALL of your global variables here; values left over from the
 * previous trace point into a heap that no longer exists.
 *
 * Returns true on success, false if the heap could not be set up.
 */
bool mm_init(void) {
    // We want the first payload to start at ALIGNMENT bytes from the start
    // of the heap, so leave room for one 8-byte header before it
    void *padding = mem_sbrk(ALIGNMENT - sizeof(size_t));
    if (padding == (void *) -1) {
        return false;
    }

    // Initialize the heap with no blocks
    mm_heap_first = NULL;
    mm_heap_last = NULL;
    free_list_head = NULL;
    return true;
}

/**
 * mm_malloc - Allocates a block with the given size (Checkpoints 1 and 2)
 *
 * Returns a pointer to a payload of at least `size` bytes, aligned to 16
 * bytes. The block must lie inside the heap and must not overlap any other
 * allocated block. Returns NULL if the heap cannot grow.
 */
void *mm_malloc(size_t size) {
    // PASTE: replace this body with the body of your Part 1 mm_malloc,
    // without the call to coalesce_free_blocks.
    size = round_up(2 * sizeof(size_t) + size, ALIGNMENT);
    if (size < MIN_BLOCK_SIZE) {
        size = MIN_BLOCK_SIZE;
    }

    block_t *block = find_fit(size);
    if (block != NULL) {
        list_remove(block);
        size_t bsize = get_size(block);

        if (bsize - size >= MIN_BLOCK_SIZE) {
            set_header(block, size, true);

            block_t *rest = (block_t *)((char *)block + size);
            set_header(rest, bsize - size, false);
            list_add(rest);

            if (block == mm_heap_last) {
                mm_heap_last = rest;
            }
        } else {
            set_header(block, bsize, true);
        }

        return block->payload;
    }

    block = mem_sbrk(size);
    if (block == (void *) -1) {
        return NULL;
    }

    if (mm_heap_first == NULL) {
        mm_heap_first = block;
    }
    mm_heap_last = block;

    set_header(block, size, true);
    return block->payload;
}

/**
 * mm_free - Releases a block to be reused for future allocations
 *      (Checkpoints 1 and 3)
 *
 * `ptr` is a payload pointer returned by an earlier mm_malloc, mm_calloc,
 * or mm_realloc that has not been freed yet. mm_free(NULL) does nothing.
 */
void mm_free(void *ptr) {
    // mm_free(NULL) does nothing
    if (ptr == NULL) {
        return;
    }

    // Mark the block as unallocated
    block_t *block = block_from_payload(ptr);
    set_header(block, get_size(block), false);
}

/**
 * mm_realloc - Change the size of the block by mm_mallocing a new block,
 *      copying its data, and mm_freeing the old block. (Checkpoint 2)
 *
 * Returns a pointer to a payload of at least `size` bytes that holds the
 * same data as the old block.
 *
 * If old_ptr is NULL, behaves like mm_malloc(size).
 * If size is 0, behaves like mm_free(old_ptr) and returns NULL.
 */
void *mm_realloc(void *old_ptr, size_t size) {
    // PASTE: replace this body with the body of your Part 1 mm_realloc.
    (void) old_ptr;
    (void) size;
    return NULL;
}

/**
 * mm_calloc - Allocate the block and set it to zero.
 *
 * Allocates an array of `nmemb` elements of `size` bytes each, with every
 * byte set to 0, and returns a pointer to it.
 */
void *mm_calloc(size_t nmemb, size_t size) {
    // PASTE: replace this body with the body of your Part 1 mm_calloc.
    (void) nmemb;
    (void) size;
    return NULL;
}

/**
 * mm_checkheap - Checks the heap for errors.
 *
 * Optional, but strongly recommended. Running the driver with -D calls
 * this before every operation. It should print nothing while the heap is
 * fine, and print a message describing the problem as soon as it finds one.
 * If you wrote a checker in Part 1, paste it here.
 *
 * Ideas for what to check:
 *   - Each block's size is a multiple of ALIGNMENT, and its header matches
 *     its footer.
 *   - Every block on the free list is marked free, and every free block in
 *     the heap is on the free list.
 *   - For every block on the free list, block->next->prev == block.
 */
void mm_checkheap(void) {

}
