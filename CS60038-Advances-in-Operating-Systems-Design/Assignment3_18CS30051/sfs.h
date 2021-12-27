#include <stdint.h>

const static uint32_t MAGIC = 12345;					 // Magic number
const static uint32_t MAX_FILE_SIZE = (5 + 1024) * 4096; // Maximum file size is 4KB + 1KB
const static uint32_t MAX_FILENAME_LENGTH = 256;		 // Maximum filename length

typedef struct inode
{
	uint32_t valid;		// 0 if invalid
	uint32_t size;		// logical size of the file
	uint32_t direct[5]; // direct data block pointer
	uint32_t indirect;	// indirect pointer
} inode;

typedef struct super_block
{
	uint32_t magic_number; // File system magic number
	uint32_t blocks;	   // Number of blocks in file system (except super block)

	uint32_t inode_blocks;			 // Number of blocks reserved for inodes == 10% of Blocks
	uint32_t inodes;				 // Number of inodes in file system == length of inode bit map
	uint32_t inode_bitmap_block_idx; // Block Number of the first inode bit map block
	uint32_t inode_block_idx;		 // Block Number of the first inode block

	uint32_t data_block_bitmap_idx; // Block number of the first data bitmap block
	uint32_t data_block_idx;		// Block number of the first data block
	uint32_t data_blocks;			// Number of blocks reserved as data blocks
} super_block;

typedef struct dir_entry
{
	int valid;	   // unset when a file or directory is deleted and can be replace by some other entry later
	int type;	   // 0 for file, 1 for directory
	char *name;	   // name of the file or directory
	int name_len;  // length of the name
	int inode_idx; // index of the inode
} dir_entry;

typedef unsigned char bitset;

int format(disk *diskptr);
int mount(disk *diskptr);
int create_file();
int remove_file(int inumber);
int stat(int inumber);
int read_i(int inumber, char *data, int length, int offset);
int write_i(int inumber, char *data, int length, int offset);
int fit_to_size(int inumber, int size);

int read_file(char *filepath, char *data, int length, int offset);
int write_file(char *filepath, char *data, int length, int offset);
int create_dir(char *dirpath);
int remove_dir(char *dirpath);

void set(bitset *bitmap, int index);
void unset(bitset *bitmap, int index);
int is_set(bitset *bitmap, int index);

int find_free_inode();
int find_free_data_block();
int clear_bitmap(int block, int bitmap_start);

char **path_parse(char *path, int *n_parts);
int get_inumber(char *path, int parent);
int find_file(int inumber, char *filename);
int recursive_create(char **dirs, int nun);
int recursive_remove(int inumber, int type);
void free_data_block(int block_idx);
void free_inode(int inumber);
int validate_path(char **path, int n_parts);