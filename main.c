#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

struct meta
{
    struct meta *next;
    uint8_t *start;
    uint8_t *end;
};

// The struct (handler) that holds the information about the allocator
struct cont_buf_handler
{
    uint8_t *buffer;
    size_t buffer_size;
    uint8_t min_block_size;
    struct meta *first;
};

// The function takes the pointer to the buffer and prepares a handler that can be used for all API calls. // buf – is the pointer to the original buffer (can be defined as: uint8_t my_buffer[512];), size – is the // buffer size (512B), min_block_size – is the minimum block size which can be allocated (8 Bytes).
struct cont_buf_handler *circ_cont_buf_init(uint8_t *buf, size_t size, uint8_t min_block_size)
{
    struct cont_buf_handler *cbuf = (struct cont_buf_handler *)malloc(sizeof(struct cont_buf_handler));

    // If we couldn't allocate
    if (!cbuf)
    {
        return NULL;
    }

    // Init cbuf
    cbuf->buffer = buf;
    cbuf->buffer_size = size;
    cbuf->min_block_size = min_block_size;

    // The first block is the first node in a linked list keeping track of memory usage
    cbuf->first = NULL;

    return cbuf;
}

// The function takes the handler and allocates the memory from the buffer with length – len. // It returns the pointer to the allocated memory.
uint8_t *circ_cont_buf_alloc(struct cont_buf_handler *cbuf, size_t len)
{
    // Meets min block size criteria?
    if (len < cbuf->min_block_size)
    {
        return NULL;
    }

    // Pointer to the last node
    struct meta *current = NULL;

    // Check to see if a meta node exists
    if (cbuf->first)
    {
        // Get the last node in the list
        current = cbuf->first;
        while (current->next)
        {
            current = current->next;
        }

        // Check to see capacity
        size_t len_free = cbuf->buffer_size - (current->end - cbuf->buffer);

        // Not enough
        if (len_free < len)
        {
            return NULL;
        }

        // Now, we know there's enough room. Just need to add another node on the list
        struct meta *next = (struct meta *)malloc(sizeof(struct meta));
        current->next = next;
        next->next = NULL;
        next->start = current->end + 1;
        next->end = next->start + len;

        // And return the right spot in the buffer
        return next->start;
    }

    else
    {
        // As nothing has been allocd, we have the whole buffer
        if (len > cbuf->buffer_size)
        {
            return NULL;
        }

        // Create the first node
        cbuf->first = (struct meta *)malloc(sizeof(struct meta));
        cbuf->first->next = NULL;
        cbuf->first->start = cbuf->buffer;
        cbuf->first->end = cbuf->first->start + len;
        // And return the right spot in the buffer
        return cbuf->first->start;
    }

    return NULL;
}

// The function peeks for the first buffer chunk (We assume that the buffer is designed as a queue or FIFO)
int circ_cont_buf_peek(struct cont_buf_handler *cbuf, uint8_t **buf)
{
    // Check to see if a meta node exists
    if (cbuf->first)
    {
        *buf = cbuf->first->start;
        return 0;
    }

    return -1;
}

// The function removes/frees the first block. The blocks will be removed in the same order as they were added!
int circ_cont_buf_free(struct cont_buf_handler *cbuf)
{
    // Check to see if a meta node exists
    if (cbuf->first)
    {
        // Get the address of the first node before advancing to the next node
        struct meta *old = cbuf->first;

        // Advance to the next node
        cbuf->first = cbuf->first->next;

        // Delete the first node
        free(old);

        return 0;
    }

    return -1;
}

void circ_cont_print_buffer_stats(struct cont_buf_handler *cbuf)
{
    printf("\ncbuf->buffer = %p\n", (void *)cbuf->buffer);

    // Check to see if a meta node exists
    if (cbuf->first)
    {
        // Get the last node in the list
        struct meta *current = cbuf->first;
        do
        {
            printf("Block\n");
            printf("loc (start) = %p\n", current->start);
            printf("loc (end) = %p\n", current->end);
            printf("len = %ld\n\n", current->end - current->start);
            current = current->next;
        } while (current);
    }

    else
    {
        printf("No blocks allocated.\n");
    }
}

int main(void)
{
    // 1. Prepare the empty buffer (call the function circ_cont_buf_init)
    uint8_t buffer[512] = {0};
    struct cont_buf_handler *cbuf = circ_cont_buf_init(buffer, 512, 8);

    // 2. Allocate 12Bytes of contiguous memory (call the function circ_cont_buf_alloc)
    uint8_t *block1 = circ_cont_buf_alloc(cbuf, 12);

    // 3. Allocate another 18Bytes (call the function circ_cont_buf_alloc)
    uint8_t *block2 = circ_cont_buf_alloc(cbuf, 18);

    // 4. At that moment if we call the function circ_cont_buf_peek, it returns the pointer to the first(head) buffer 12B (red buffer).
    uint8_t * peek = NULL;
    circ_cont_buf_peek(cbuf, &peek);

    if(peek == cbuf->buffer)
    {
        printf("PASS. peek == cbuf->buffer\n");
    }

    else
    {
        printf("FAIL. peek != cbuf->buffer\n");
    }

    // 5. Free the first allocated buffer (call the function circ_cont_buf_free)
    circ_cont_buf_free(cbuf);

    // 6. If we call the function circ_cont_buf_peek again, then it returns the pointer to the green buffer 18B.
    uint8_t * peek2 = NULL;
    circ_cont_buf_peek(cbuf, &peek2);

    if(peek2 == cbuf->buffer)
    {
        printf("FAIL. The first (head) is at the loc of cbuf->buffer\n");
    }

    else
    {
        printf("PASS. The first (head) is not that the loc of cbuf->buffer\n");
    }

    // 7. Another allocations will reserve more blocks using free space in the buffer. If there is no space, then the function circ_cont_buf_alloc returns error.
    uint8_t *block3 = circ_cont_buf_alloc(cbuf, 1000);

    if(block3)
    {
        printf("FAIL. 1000B alloc should not have been successful.\n");
    }

    else
    {
        printf("PASS. 1000B alloc was not successful.\n");
    }

    return 0;
}