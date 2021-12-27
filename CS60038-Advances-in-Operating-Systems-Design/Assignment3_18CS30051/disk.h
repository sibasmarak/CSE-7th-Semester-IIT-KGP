#include <stdint.h>

const static int BLOCKSIZE = 4 * 1024;
const static int STATSSIZE = 24;

typedef struct disk
{
    uint32_t size;    // size of the disk
    uint32_t blocks;  // number of usable blocks (except stat block)
    uint32_t reads;   // number of block reads performed
    uint32_t writes;  // number of block writes performed
    char **block_arr; // array (of size blocks) of pointers to 4KB blocks
} disk;

disk *create_disk(int nbytes);
int read_block(disk *diskptr, int blocknr, void *block_data);
int write_block(disk *diskptr, int blocknr, void *block_data);
int free_disk(disk *diskptr);