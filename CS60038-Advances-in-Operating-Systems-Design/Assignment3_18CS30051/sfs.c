/*
------------------------------------------
Assignment 3
------------------------------------------
subject:        Advanced Operating Systems
year:           2021 - 2022
------------------------------------------
Name:           Debajyoti Dasgupta
Roll No:        18CS30051
------------------------------------------
Kernel Version: 5.11.0-37-generic
System:         Ubuntu 20.04.3 LTS
------------------------------------------
File Name       sfs.c
------------------------------------------
*/

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "disk.h"
#include "sfs.h"

static disk *mounted_disk;
static super_block *sb;
int root_file_inode = 0;

int format(disk *diskptr)
{
    if (diskptr == NULL)
    {
        printf("{SFS} -- [ERROR] Disk is not initialized -- Disk is not formatted\n");
        return -1;
    }

    int N = diskptr->blocks;
    int M = N - 1;
    int I = (int)(0.1 * M);
    int n_inodes = I * 128;
    int IB = (int)ceil(n_inodes / (8. * BLOCKSIZE));

    int R = M - I - IB;
    int DBB = (int)ceil(R / (8. * BLOCKSIZE));
    int DB = R - DBB;

    // Initialize the super block
    if (sb == NULL)
        sb = (super_block *)malloc(sizeof(super_block));
    sb->magic_number = MAGIC;
    sb->blocks = M;
    sb->inode_blocks = I;
    sb->inodes = n_inodes;
    sb->inode_bitmap_block_idx = 1;
    sb->inode_block_idx = 1 + IB + DBB;
    sb->data_block_bitmap_idx = 1 + IB;
    sb->data_block_idx = 1 + IB + DBB + I;
    sb->data_blocks = DB;

    // Write the super block
    if (write_block(diskptr, 0, sb))
    {
        printf("{SFS} -- [ERROR] Failed to write super block -- Disk is not formatted\n");
        return -1;
    }

    // Initialize the inode bit map
    bitset *inode_bitmap = (bitset *)malloc(IB * BLOCKSIZE);
    memset(inode_bitmap, 0, IB * BLOCKSIZE);

    // Write inode bit map to disk
    for (int i = 0; i < IB; i++)
    {
        int ret = write_block(diskptr, sb->inode_bitmap_block_idx + i, inode_bitmap + i * BLOCKSIZE);
        if (ret < 0)
        {
            printf("{SFS} -- [ERROR]: Error writing inode bitmap to disk -- Disk is not formatted\n");
            return -1;
        }
    }

    // Initialize the data block bit map
    bitset *data_bitmap = (char *)malloc(DBB * BLOCKSIZE);
    memset(data_bitmap, 0, DBB * BLOCKSIZE);

    // Write data block bit map to disk
    for (int i = 0; i < DBB; i++)
    {
        int ret = write_block(diskptr, sb->data_block_bitmap_idx + i, data_bitmap + i);
        if (ret < 0)
        {
            printf("{SFS} -- [ERROR]: Error writing data bitmap to disk -- Disk is not formatted\n");
            return -1;
        }
    }

    // Initialize the inodes
    inode *inodes = (inode *)malloc(128 * sizeof(inode));
    for (int i = 0; i < 128; i++)
    {
        inodes[i].valid = 0;
    }

    // Write the inodes
    for (int i = 0; i < sb->inode_blocks; i++)
    {
        write_block(diskptr, sb->inode_block_idx + i, inodes);
    }

    // Free the memory
    free(inodes);
    free(inode_bitmap);
    free(data_bitmap);

    return 0;
}

int mount(disk *diskptr)
{
    if (diskptr == NULL)
    {
        printf("{SFS} -- [ERROR] Disk is not initialized -- Disk is not mounted\n");
        return -1;
    }

    // Read the super block
    if (sb == NULL)
        sb = (super_block *)malloc(sizeof(super_block));
    if (read_block(diskptr, 0, sb) < 0)
    {
        printf("{SFS} -- [ERROR] Failed to read super block -- Disk is not mounted\n");
        return -1;
    }

    // Check the magic number
    if (sb->magic_number != MAGIC)
    {
        printf("{SFS} -- [ERROR] Magic number is incorrect -- Disk is not mounted\n");
        return -1;
    }

    mounted_disk = diskptr;

    // Create the root directory
    root_file_inode = create_file();
    return 0;
}

int create_file()
{
    if (mounted_disk == NULL)
    {
        printf("{SFS} -- [ERROR] Disk is not mounted -- File not created\n");
        return -1;
    }

    // Find the first free inode
    int inode_idx = find_free_inode();
    if (inode_idx < 0)
    {
        printf("{SFS} -- [ERROR] No free inodes -- File not created\n");
        return -1;
    }

    // Initialize the inode
    int inode_block_idx = sb->inode_block_idx + (inode_idx / 128);
    int inode_offset = (inode_idx % 128);

    inode *inode_ptr = (inode *)malloc(128 * sizeof(inode));
    read_block(mounted_disk, inode_block_idx, inode_ptr);

    inode_ptr[inode_offset].valid = 1;
    inode_ptr[inode_offset].size = 0;

    // Write the inode to disk
    if (write_block(mounted_disk, inode_block_idx, inode_ptr))
    {
        printf("{SFS} -- [ERROR] Failed to write inode to disk -- File not created\n");
        return -1;
    }

    return inode_idx;
}

int remove_file(int inumber)
{
    if (mounted_disk == NULL)
    {
        printf("{SFS} -- [ERROR] Disk is not mounted -- File not removed\n");
        return -1;
    }

    // Read the inode
    int inode_block_idx = sb->inode_block_idx + (inumber / 128);
    int inode_offset = (inumber % 128);

    inode *inode_ptr = (inode *)malloc(128 * sizeof(inode));
    read_block(mounted_disk, inode_block_idx, inode_ptr);

    // Check if the inode is valid
    if (inode_ptr[inode_offset].valid == 0)
    {
        printf("{SFS} -- [ERROR] Inode is not valid -- File not removed\n");
        return -1;
    }

    // Clear the data bitmap
    int data_block_bitmap_idx = sb->data_block_bitmap_idx;
    int file_blocks = (int)ceil((double)inode_ptr[inode_offset].size / BLOCKSIZE);

    uint32_t *indirect_ptr = NULL;
    for (int i = 0; i < file_blocks; i++)
    {
        if (i < 5)
        {
            if (clear_bitmap(inode_ptr[inode_offset].direct[i], data_block_bitmap_idx))
            {
                printf("{SFS} -- [ERROR] Failed to clear data bitmap -- File not removed\n");
                return -1;
            }
        }
        else
        {
            if (indirect_ptr == NULL)
            {
                indirect_ptr = (uint32_t *)malloc(BLOCKSIZE);
                if (read_block(mounted_disk, sb->data_block_idx + inode_ptr[inode_offset].indirect, indirect_ptr) < 0)
                {
                    printf("{SFS} -- [ERROR] Failed to read indirect block -- File not removed\n");
                    return -1;
                }

                if (clear_bitmap(inode_ptr[inode_offset].indirect, data_block_bitmap_idx))
                {
                    printf("{SFS} -- [ERROR] Failed to clear data bitmap -- File not removed\n");
                    return -1;
                }
            }
            if (clear_bitmap(indirect_ptr[i - 5], data_block_bitmap_idx))
            {
                printf("{SFS} -- [ERROR] Failed to clear data bitmap -- File not removed\n");
                return -1;
            }
        }
    }

    // Free the inode
    inode_ptr[inode_offset].valid = 0;
    inode_ptr[inode_offset].size = 0;

    // Write the inode to disk
    if (write_block(mounted_disk, inode_block_idx, inode_ptr))
    {
        printf("{SFS} -- [ERROR] Failed to write inode to disk -- File not removed\n");
        return -1;
    }

    // Clere the inode bitmap
    if (clear_bitmap(inumber, sb->inode_bitmap_block_idx))
    {
        printf("{SFS} -- [ERROR] Failed to clear inode bitmap -- File not removed\n");
        return -1;
    }

    return 0;
}

int stat(int inumber)
{
    if (mounted_disk == NULL)
    {
        printf("{SFS} -- [ERROR] Disk is not mounted -- Stat not printed\n");
        return -1;
    }

    // Read the inode
    int inode_block_idx = sb->inode_block_idx + (inumber / 128);
    int inode_offset = (inumber % 128);

    inode *inode_ptr = (inode *)malloc(128 * sizeof(inode));
    read_block(mounted_disk, inode_block_idx, inode_ptr);

    // Check if the inode is valid
    if (inode_ptr[inode_offset].valid == 0)
    {
        inode_ptr[inode_offset].size = 0;
    }

    int blocks = (int)ceil((double)inode_ptr[inode_offset].size / BLOCKSIZE);
    int direct_blocks = (blocks < 5) ? blocks : 5;
    int indirect_blocks = blocks - direct_blocks;

    printf("\n\
    +---------------------[FILE STAT]---------------------+\n\
    |                                                     |\n\
    |  Valid:                              %8d       |\n\
    |  File Size (Logical):                %8d       |\n\
    |  Number of Data Blocks in use:       %8d       |\n\
    |  Number of Direct Pointers in use:   %8d       |\n\
    |  Number of Indirect Pointers in use: %8d       |\n\
    |                                                     |\n\
    +-----------------------------------------------------+\n\n",
           inode_ptr[inode_offset].valid,
           inode_ptr[inode_offset].size,
           blocks,
           direct_blocks,
           indirect_blocks);

    return 0;
}

int read_i(int inumber, char *data, int length, int offset)
{
    if (mounted_disk == NULL)
    {
        printf("{SFS} -- [ERROR] Disk is not mounted -- File not read\n");
        return -1;
    }

    // Read the inode
    int inode_block_idx = sb->inode_block_idx + (inumber / 128);
    int inode_offset = (inumber % 128);

    inode *inode_ptr = (inode *)malloc(128 * sizeof(inode));
    read_block(mounted_disk, inode_block_idx, inode_ptr);

    // Check if the inode is valid
    if (inode_ptr[inode_offset].valid == 0)
    {
        printf("{SFS} -- [ERROR] Inode is not valid -- File not read\n");
        return -1;
    }

    if (length == 0)
    {
        return 0;
    }

    // Check if the offset is valid
    if (offset < 0 || offset >= inode_ptr[inode_offset].size)
    {
        printf("{SFS} -- [ERROR] Offset is not valid -- File not read\n");
        return -1;
    }

    // Read the data
    int start_block = offset / BLOCKSIZE;

    //truncate the length if it is too long
    int final_length = (length + offset > inode_ptr[inode_offset].size ? inode_ptr[inode_offset].size : length + offset);
    int end_block = (final_length + BLOCKSIZE - 1) / BLOCKSIZE;
    length = final_length - offset;

    int bytes_read = 0;
    char *temp_data = (char *)malloc(BLOCKSIZE);
    int32_t *indirect_ptr = (int32_t *)malloc(BLOCKSIZE);
    int file_blocks = (int)ceil((double)inode_ptr[inode_offset].size / BLOCKSIZE);

    if (file_blocks > 5)
        read_block(mounted_disk, sb->data_block_idx + inode_ptr[inode_offset].indirect, (char *)indirect_ptr);

    int data_offset = 0;
    for (int i = start_block; i < end_block; i++)
    {
        if (i < 5)
        {
            if (read_block(mounted_disk, sb->data_block_idx + inode_ptr[inode_offset].direct[i], temp_data) < 0)
            {
                printf("{SFS} -- [ERROR] Failed to read data block -- File not read\n");
                return -1;
            }
        }
        else
        {
            if (read_block(mounted_disk, sb->data_block_idx + indirect_ptr[i - 5], temp_data) < 0)
            {
                printf("{SFS} -- [ERROR] Failed to read data block -- File not read\n");
                return -1;
            }
        }

        int copy_length = (i == end_block - 1 ? length : BLOCKSIZE - offset % BLOCKSIZE);
        memcpy(data + data_offset, temp_data + offset % BLOCKSIZE, copy_length);

        data_offset += copy_length;
        offset += copy_length;
        bytes_read += copy_length;
        length -= copy_length;
    }

    free(temp_data);
    free(indirect_ptr);
    return bytes_read;
}

int write_i(int inumber, char *data, int length, int offset)
{
    // if (length == 24)
    // {
    //     // print dir_entry
    //     dir_entry *dir_entry_ptr = (dir_entry *)data;

    //     printf("+----------[DIR ENTRY]--------------+\n");
    //     printf("|  Name:  %20s      |\n", dir_entry_ptr->name);
    //     printf("|  Inode: %20d      |\n", dir_entry_ptr->inode_idx);
    //     printf("|  Valid: %20d      |\n", dir_entry_ptr->valid);
    //     printf("|  Type:  %20d      |\n", dir_entry_ptr->type);
    //     printf("+-----------------------------------+\n");
    // }

    if (mounted_disk == NULL)
    {
        printf("{SFS} -- [ERROR] Disk is not mounted -- File not write\n");
        return -1;
    }

    if (length == 0)
    {
        return 0;
    }

    // Read the inode
    int inode_block_idx = sb->inode_block_idx + (inumber / 128);
    int inode_offset = (inumber % 128);

    inode *inode_ptr = (inode *)malloc(128 * sizeof(inode));
    read_block(mounted_disk, inode_block_idx, inode_ptr);

    // Check if the inode is valid
    if (inode_ptr[inode_offset].valid == 0)
    {
        printf("{SFS} -- [ERROR] Inode is not valid -- File not write\n");
        return -1;
    }

    // Check if the offset is valid
    if (offset < 0)
    {
        printf("{SFS} -- [ERROR] Offset is not valid -- File not write\n");
        return -1;
    }

    // Read the data
    int start_block = offset / BLOCKSIZE;

    //truncate the length if it is too long
    int final_length = (length + offset > MAX_FILE_SIZE ? MAX_FILE_SIZE : length + offset);
    int end_block = (final_length + BLOCKSIZE - 1) / BLOCKSIZE;
    length = final_length - offset;
    int32_t *indirect_ptr = (int32_t *)malloc(BLOCKSIZE);

    int file_blocks = (inode_ptr[inode_offset].size + BLOCKSIZE - 1) / BLOCKSIZE;
    int req_blocks = (final_length + BLOCKSIZE - 1) / BLOCKSIZE;

    int bytes_written = 0, data_offset = 0;
    if (file_blocks < req_blocks)
    {
        int empty_blocks = start_block - file_blocks - 1;
        if (empty_blocks < 0)
            empty_blocks = 0;
        bytes_written += empty_blocks * BLOCKSIZE;
        bytes_written += offset % BLOCKSIZE;

        for (int i = file_blocks + (inode_ptr[inode_offset].size != 0); i < req_blocks; i++)
        {
            if (i < 5)
                inode_ptr[inode_offset].direct[i] = find_free_data_block();
            else
            {
                if (i == 5)
                {
                    inode_ptr[inode_offset].indirect = find_free_data_block();
                    write_block(mounted_disk, inode_block_idx, inode_ptr);
                }
                read_block(mounted_disk, sb->data_block_idx + inode_ptr[inode_offset].indirect, (char *)indirect_ptr);
                indirect_ptr[i - 5] = find_free_data_block();
                write_block(mounted_disk, sb->data_block_idx + inode_ptr[inode_offset].indirect, (char *)indirect_ptr);
            }
        }
    }

    char *temp_data = (char *)malloc(BLOCKSIZE);
    read_block(mounted_disk, sb->data_block_idx + inode_ptr[inode_offset].indirect, (char *)indirect_ptr);

    for (int i = start_block; i < end_block; i++)
    {
        if (i < 5)
        {
            if (read_block(mounted_disk, sb->data_block_idx + inode_ptr[inode_offset].direct[i], temp_data) < 0)
            {
                printf("{SFS} -- [ERROR] Failed to read direct data block -- File not written\n");
                return -1;
            }
        }
        else
        {
            if (read_block(mounted_disk, indirect_ptr[i - 5], temp_data) < 0)
            {
                printf("{SFS} -- [ERROR] Failed to read indirect data block -- File not written\n");
                return -1;
            }
        }

        int copy_length = (i == end_block - 1 ? length : BLOCKSIZE - offset % BLOCKSIZE);
        memcpy(temp_data + offset % BLOCKSIZE, data + data_offset, copy_length);

        if (i < 5)
            write_block(mounted_disk, sb->data_block_idx + inode_ptr[inode_offset].direct[i], temp_data);
        else
            write_block(mounted_disk, sb->data_block_idx + indirect_ptr[i - 5], temp_data);

        // printf("\033[1;35m[DEBUG]\033[0m Written [%5d] bytes to block [%2d] at offset [%5d]\n", copy_length, i, offset % BLOCKSIZE);
        data_offset += copy_length;
        offset += copy_length;
        bytes_written += copy_length;
        length -= copy_length;
    }

    // Update the inode
    inode_ptr[inode_offset].size = final_length;
    write_block(mounted_disk, inode_block_idx, inode_ptr);

    free(temp_data);
    free(indirect_ptr);
    free(inode_ptr);

    return bytes_written;
}

int fit_to_size(int inumber, int size)
{
    if (mounted_disk == NULL)
    {
        printf("{SFS} -- [ERROR] Disk is not mounted -- File not fit_to_size\n");
        return -1;
    }

    // Read the inode
    int inode_block_idx = sb->inode_block_idx + (inumber / 128);
    int inode_offset = (inumber % 128);

    inode *inode_ptr = (inode *)malloc(128 * sizeof(inode));
    if (read_block(mounted_disk, inode_block_idx, inode_ptr) < 0)
    {
        printf("{SFS} -- [ERROR] Failed to read inode from disk -- File not fit_to_size\n");
        return -1;
    }

    // Check if the inode is valid
    if (inode_ptr[inode_offset].valid == 0)
    {
        printf("{SFS} -- [ERROR] Inode is not valid -- File not fit_to_size\n");
        return -1;
    }

    int file_blocks = (inode_ptr[inode_offset].size + BLOCKSIZE - 1) / BLOCKSIZE;

    if (inode_ptr[inode_offset].size > size)
    {
        inode_ptr[inode_offset].size = size;
        if (write_block(mounted_disk, inode_block_idx, inode_ptr))
        {
            printf("{SFS} -- [ERROR] Failed to write inode to disk -- File not fit_to_size\n");
            return -1;
        }
    }

    int req_blocks = (size + BLOCKSIZE - 1) / BLOCKSIZE;

    int free_indirect = 0;
    for (int i = req_blocks + 1; i <= file_blocks; i++)
    {
        if (i < 5)
            free_data_block(inode_ptr[inode_offset].direct[i]);
        else
        {
            if (i == 5)
                free_indirect = 1;
            int32_t *indirect_ptr = (int32_t *)malloc(BLOCKSIZE);
            read_block(mounted_disk, sb->data_block_idx + inode_ptr[inode_offset].indirect, (char *)indirect_ptr);
            free_data_block(indirect_ptr[i - 5]);
            write_block(mounted_disk, sb->data_block_idx + inode_ptr[inode_offset].indirect, (char *)indirect_ptr);
        }
    }
    if (free_indirect == 1)
    {
        free_data_block(inode_ptr[inode_offset].indirect);
        inode_ptr[inode_offset].indirect = find_free_data_block();
        write_block(mounted_disk, inode_block_idx, inode_ptr);
    }

    return 0;
}

int create_dir(char *dirpath)
{

    if (mounted_disk == NULL)
    {
        printf("{SFS} -- [ERROR] Disk is not mounted -- Directory not created\n");
        return -1;
    }

    // Get the name of the directory to be created
    int *n_parts = (int *)malloc(sizeof(int));
    char **parts = path_parse(dirpath, n_parts);

    if (!validate_path(parts, *n_parts))
    {
        printf("{SFS} -- [ERROR] Invalid path -- Directory not created\n");
        return -1;
    }

    int new_inumber = -1;
    if ((new_inumber = recursive_create(parts, *n_parts)) == -1)
    {
        printf("{SFS} -- [ERROR] Failed to create directory -- Directory not created \n");
        return -1;
    }

    free(n_parts);
    free(parts);
    return new_inumber;
}

int remove_dir(char *dirpath)
{

    if (mounted_disk == NULL)
    {
        printf("{SFS} -- [ERROR] Disk is not mounted -- Directory not removed\n");
        return -1;
    }

    int parent_inumber = get_inumber(dirpath, 1);
    if (parent_inumber == -1)
    {
        printf("{SFS} -- [ERROR] Directory not found -- Directory not removed\n");
        return -1;
    }

    // Get the name of the directory to be removed

    int *n_parts = (int *)malloc(sizeof(int));
    char **parts = path_parse(dirpath, n_parts);
    char *dirname = parts[*n_parts - 1];

    if (!validate_path(parts, *n_parts))
    {
        printf("{SFS} -- [ERROR] Invalid path -- Directory not removed\n");
        return -1;
    }

    // Check if the directory already exists
    if (find_file(parent_inumber, dirname) < 0)
    {
        printf("{SFS} -- [ERROR] Directory does not exists -- Directory not removed\n");
        return -1;
    }

    // read parent inode to compute offset
    inode *parent_inode = (inode *)malloc(BLOCKSIZE);
    int offset = parent_inumber % 128;

    if (read_block(mounted_disk, sb->inode_block_idx + parent_inumber / 128, parent_inode) < 0)
    {
        printf("{SFS} -- [ERROR] Failed to read inode -- Directory not removed\n");
        return -1;
    }

    int file_size = parent_inode[offset].size;

    dir_entry *dir_files = (dir_entry *)malloc(file_size);
    if (read_i(parent_inumber, (char *)dir_files, file_size, 0) < 0)
    {
        printf("{SFS} -- [ERROR] Failed to read data block -- Directory not removed\n");
        return -1;
    }

    for (int i = 0; i < file_size / sizeof(dir_entry); i++)
    {
        if (strcmp(dirname, dir_files[i].name) == 0)
        {
            dir_files[i].valid = 0;

            if (recursive_remove(dir_files[i].inode_idx, dir_files[i].type) < 0)
            {
                printf("{SFS} -- [ERROR] Failed to recursively remove directory -- Directory not removed\n");
                return -1;
            }

            break;
        }
    }

    // Write the directory entry
    if (write_i(parent_inumber, (char *)dir_files, file_size, 0) < 0)
    {
        printf("{SFS} -- [ERROR] Failed to write data block -- Directory not removed\n");

        free(dir_files);
        free(parent_inode);
        return -1;
    }

    free(dir_files);
    free(parent_inode);
    return 0;
}
int read_file(char *filepath, char *data, int length, int offset)

{
    int inumber = get_inumber(filepath, 0);
    if (inumber == -1)
    {
        printf("{SFS} -- [ERROR] File not found -- File not read\n");
        return -1;
    }

    if (data == NULL)
    {
        printf("{SFS} -- [ERROR] Invalid data pointer -- Failed Read File\n");
        return -1;
    }

    // Read data from the inode in data buffer
    int bytes_read = read_i(inumber, data, length, offset);
    if (bytes_read == -1)
    {
        printf("{SFS} -- [ERROR] Failed to read file -- File not read\n");
        return -1;
    }

    return bytes_read;
}

int write_file(char *filepath, char *data, int length, int offset)
{
    int parent_inumber = get_inumber(filepath, 1);
    if (parent_inumber == -1)
    {
        printf("{SFS} -- [ERROR] File not found -- File not written\n");
        return -1;
    }

    if (data == NULL)
    {
        printf("{SFS} -- [ERROR] Invalid data pointer -- Failed Write File\n");
        return -1;
    }

    // Get the name of the File to be created
    int *n_parts = (int *)malloc(sizeof(int));
    char **parts = path_parse(filepath, n_parts);
    char *filename = parts[*n_parts - 1];

    if (!validate_path(parts, *n_parts))
    {
        printf("{SFS} -- [ERROR] Invalid path -- File not written\n");
        return -1;
    }

    // Check if the File already exists
    int inumber = -1;
    if ((inumber = find_file(parent_inumber, filename)) < 0)
    {
        if ((inumber = create_file()) < 0)
        {
            printf("{SFS} -- [ERROR] Failed to write file -- File not written\n");
            return -1;
        }
    }

    // Write data to the inode in data buffer
    int bytes_written = write_i(inumber, data, length, offset);
    if (bytes_written == -1)
    {
        printf("{SFS} -- [ERROR] Failed to write file -- File not written\n");
        return -1;
    }

    dir_entry *e = (dir_entry *)malloc(sizeof(dir_entry));
    e->inode_idx = inumber;
    e->name = (char *)malloc(strlen(filename) + 1);
    strcpy(e->name, filename);
    e->name_len = strlen(filename);
    e->type = 0;
    e->valid = 1;

    // Write the directory entry
    inode *parent_inode = (inode *)malloc(BLOCKSIZE);
    int off = parent_inumber % 128;
    if (read_block(mounted_disk, sb->inode_block_idx + parent_inumber / 128, parent_inode))
    {
        printf("{SFS} -- [ERROR] Failed to read inode -- File not written\n");
        return -1;
    }

    int file_size = parent_inode[off].size;

    dir_entry *dir_files = (dir_entry *)malloc(file_size);
    if (read_i(parent_inumber, (char *)dir_files, file_size, 0) < 0)
    {
        printf("{SFS} -- [ERROR] Failed to read data block -- File not written\n");
        return -1;
    }

    for (int i = 0; i < file_size / sizeof(dir_entry); i++)
    {
        if (strcmp(filename, dir_files[i].name) == 0)
        {
            dir_files[i] = *e;
            if (write_i(parent_inumber, (char *)dir_files, file_size, 0) < 0)
            {
                printf("{SFS} -- [ERROR] Failed to write data block -- File not written\n");
                return -1;
            }
            goto write_file_end;
        }
    }

    // Add the directory entry
    if (write_i(parent_inumber, (char *)e, sizeof(dir_entry), file_size) < 0)
    {
        printf("{SFS} -- [ERROR] Failed to write data block -- File not written\n");
        return -1;
    }

write_file_end:

    free(dir_files);
    free(parent_inode);
    return bytes_written;
}

void set(bitset *bitmap, int index)
{
    int byte_index = index >> 3;
    int bit_index = index & 7;
    bitmap[byte_index] |= (1 << bit_index);
}

void unset(bitset *bitmap, int index)
{
    int byte_index = index >> 3;
    int bit_index = index & 7;
    bitmap[byte_index] &= ~(1 << bit_index);
}

int is_set(bitset *bitmap, int index)
{
    int byte_index = index >> 3;
    int bit_index = index & 7;
    return (bitmap[byte_index] & (1 << bit_index)) != 0;
}

int clear_bitmap(int block, int bitmap_start)
{
    int block_number = block / (8 * BLOCKSIZE);
    int bit_number = block % (8 * BLOCKSIZE);

    bitset *bitmap = (bitset *)malloc(BLOCKSIZE);
    if (read_block(mounted_disk, bitmap_start + block_number, bitmap))
    {
        printf("{SFS} -- [ERROR] Failed to read bitmap -- Failed to clear bitmap\n");
        free(bitmap);
        return -1;
    }

    unset(bitmap, bit_number);

    if (write_block(mounted_disk, bitmap_start + block_number, bitmap))
    {
        printf("{SFS} -- [ERROR] Failed to write bitmap -- Failed to clear bitmap\n");
        free(bitmap);
        return -1;
    }

    free(bitmap);
    return 0;
}

int find_free_inode()
{
    if (mounted_disk == NULL)
    {
        printf("{SFS} -- [ERROR] Disk is not mounted -- Failed to find free inode\n");
        return -1;
    }

    // Read the inode bit map
    int IB = (int)ceil((double)sb->inodes / (BLOCKSIZE * 8));
    bitset *inode_bitmap = (bitset *)malloc(IB * BLOCKSIZE);
    for (int i = 0; i < IB; i++)
    {
        if (read_block(mounted_disk, sb->inode_bitmap_block_idx + i, inode_bitmap + i * BLOCKSIZE))
        {
            printf("{SFS} -- [ERROR] Failed to read inode bitmap -- Failed to find free inode\n");
            free(inode_bitmap);
            return -1;
        }
    }

    // Find the first free inode
    for (int i = 0; i < sb->inodes; i++)
    {
        int index = (i / BLOCKSIZE) >> 3;
        if (is_set(inode_bitmap + index * BLOCKSIZE, i % (BLOCKSIZE * 8)))
        {
            continue;
        }
        else
        {

            //  Update the inode bitmap
            int bit_idx = (i / BLOCKSIZE) >> 3;
            int bit_offset = i % (BLOCKSIZE * 8);

            set(inode_bitmap + bit_idx, bit_offset);
            if (write_block(mounted_disk, sb->inode_bitmap_block_idx + bit_idx, inode_bitmap + bit_idx))
            {
                printf("{SFS} -- [ERROR] Failed to write inode bitmap to disk\n");
                return -1;
            }
            free(inode_bitmap);
            return i;
        }
    }

    free(inode_bitmap);
    return -1;
}

int find_free_data_block()
{
    if (mounted_disk == NULL)
    {
        printf("{SFS} -- [ERROR] Disk is not mounted -- Failed to find free data block\n");
        return -1;
    }

    // Read the data block bit map
    int DB = sb->data_blocks;
    int DBB = (int)ceil((double)DB / (BLOCKSIZE * 8));
    bitset *data_block_bitmap = (bitset *)malloc(DBB * BLOCKSIZE);

    for (int i = 0; i < DBB; i++)
    {
        if (read_block(mounted_disk, sb->data_block_bitmap_idx + i, data_block_bitmap + i * BLOCKSIZE))
        {
            printf("{SFS} -- [ERROR] Failed to read data block bitmap -- Failed to find free data block\n");
            free(data_block_bitmap);
            return -1;
        }
    }

    // Find the first free data block
    for (int i = 0; i < DB; i++)
    {
        int index = (i / BLOCKSIZE) >> 3;
        if (is_set(data_block_bitmap + index * BLOCKSIZE, i % (BLOCKSIZE * 8)))
        {
            continue;
        }
        else
        {
            //  Update the data bitmap
            int bit_idx = (i / BLOCKSIZE) >> 3;
            int bit_offset = i % (BLOCKSIZE * 8);

            set(data_block_bitmap + bit_idx, bit_offset);
            if (write_block(mounted_disk, sb->data_block_bitmap_idx + bit_idx, data_block_bitmap + bit_idx))
            {
                printf("{SFS} -- [ERROR] Failed to write data block bitmap to disk\n");
                return -1;
            }

            free(data_block_bitmap);
            return i;
        }
    }

    free(data_block_bitmap);
    return -1;
}

int find_file(int inumber, char *filename)
{
    // Read the inode
    inode *in = (inode *)malloc(BLOCKSIZE);
    int off = inumber % 128;

    if (read_block(mounted_disk, sb->inode_block_idx + inumber / 128, in))
    {
        printf("{SFS} -- [ERROR] Failed to read inode -- Failed to get inumber\n");
        return -1;
    }

    if (in[off].valid == 0)
    {
        printf("{SFS} -- [ERROR] Invalid inode [%d] -- Failed to get inumber\n", inumber);
        return -1;
    }

    // Read the data block
    dir_entry *dir = (dir_entry *)malloc(BLOCKSIZE);
    int len = read_i(inumber, (char *)dir, in[off].size, 0);
    free(in);

    if (len < 0)
    {
        printf("{SFS} -- [ERROR] Failed to read data block -- Failed to get inumber\n");
        return -1;
    }

    int num_entries = len / sizeof(dir_entry);
    int child_inumber = -1;
    for (int j = 0; j < num_entries; j++)
    {
        if (strcmp(filename, dir[j].name) == 0 && dir[j].valid == 1)
        {
            child_inumber = dir[j].inode_idx;
            break;
        }
    }

    free(dir);
    return child_inumber;
}

int get_inumber(char *filepath, int parent)
{
    if (mounted_disk == NULL)
    {
        printf("{SFS} -- [ERROR] Disk is not mounted -- Failed to get inumber\n");
        return -1;
    }

    int start_inode = sb->inode_block_idx;

    int *n_parts = (int *)malloc(sizeof(int));
    char **files = path_parse(filepath, n_parts);

    if (files == NULL || *n_parts == 0)
    {
        return 0;
    }

    int inumber = 0;
    for (int i = 0; i < *n_parts - parent; i++)
    {
        inumber = find_file(inumber, files[i]);

        if (inumber < 0)
        {
            printf("{SFS} -- [ERROR] File or Directory [%s] not found -- Failed to get inumber\n", files[i]);
            free(files);
            return -1;
        }

        free(files[i]);
    }

    free(files);
    return inumber;
}

char **path_parse(char *path, int *n_parts)
{
    int len = strlen(path);

    if (len > 0 && path[0] == '/')
        path++, len--;

    if (len > 1 && path[len - 1] == '/')
    {
        path[len - 1] = '\0';
    }

    *n_parts = 1, len = strlen(path);
    for (int i = 0; i < len; ++i)
        *n_parts += (path[i] == '/') ? 1 : 0;

    char **parts = (char **)malloc(sizeof(char *) * *n_parts);
    for (int i = 0; i < *n_parts; i++)
        parts[i] = (char *)malloc(sizeof(char) * MAX_FILENAME_LENGTH);

    if (len == 0)
    {
        *n_parts = 0;
        return parts;
    }

    int j = 0;
    for (int i = 0; i < *n_parts; i++)
    {
        int k = 0;
        while (k < MAX_FILENAME_LENGTH && j < len && path[j] != '/')
        {
            parts[i][k] = path[j];
            k++, j++;
        }
        if (k >= MAX_FILENAME_LENGTH)
        {
            printf("{SFS} -- [ERROR] Filename too long -- Failed to parse path\n");
            *n_parts = -1;
            return parts;
        }
        parts[i][k] = '\0';
        j++;
    }

    return parts;
}

int recursive_create(char **dirs, int nun)
{
    if (nun == 0 || dirs == NULL)
    {
        return 0;
    }

    int inumber = 0;
    for (int x = 0; x < nun; x++)
    {
        int new_inumber = -1;
        if ((new_inumber = find_file(inumber, dirs[x])) < 0)
        {
            dir_entry *dir = (dir_entry *)malloc(sizeof(dir_entry));
            dir->inode_idx = create_file();
            new_inumber = dir->inode_idx;

            if (dir->inode_idx == -1)
            {
                printf("{SFS} -- [ERROR] No free space -- Directory not created\n");
                return -1;
            }

            dir->type = 1;
            dir->valid = 1;
            dir->name = (char *)malloc(MAX_FILENAME_LENGTH);
            strcpy(dir->name, dirs[x]);
            dir->name_len = strlen(dirs[x]);

            // read parent inode to compute offset
            inode *parent_inode = (inode *)malloc(BLOCKSIZE);
            int offset = inumber % 128;

            if (read_block(mounted_disk, sb->inode_block_idx + inumber / 128, parent_inode) < 0)
            {
                printf("{SFS} -- [ERROR] Failed to read inode -- Directory not created\n");
                return -1;
            }

            int file_size = parent_inode[offset].size;

            dir_entry *dir_files = (dir_entry *)malloc(file_size);
            if (read_i(inumber, (char *)dir_files, file_size, 0) < 0)
            {
                printf("{SFS} -- [ERROR] Failed to read data block -- Directory not created\n");
                return -1;
            }

            int found = 0;
            for (int i = 0; i < file_size / sizeof(dir_entry); i++)
            {
                if (dir_files[i].valid == 0)
                {
                    dir_files[i] = *dir;
                    if (write_i(inumber, (char *)dir_files, file_size, 0) < 0)
                    {
                        printf("{SFS} -- [ERROR] Failed to write data block -- Directory not created\n");
                        return -1;
                    }
                    found = 1;
                    break;
                }
            }

            // Write the directory entry
            if (!found)
                if (write_i(inumber, (char *)dir, sizeof(dir_entry), file_size) < 0)
                {
                    printf("{SFS} -- [ERROR] Failed to write data block -- Directory not created\n");

                    free(dir);
                    free(dir_files);
                    free(parent_inode);
                    return -1;
                }

            free(dir);
            free(dir_files);
            free(parent_inode);
        }
        inumber = new_inumber;
    }
    return inumber;
}

int recursive_remove(int inumber, int type)
{

    if (type)
    {
        // This is a Directory, so remove recursvely

        // Read the inode
        inode *in = (inode *)malloc(BLOCKSIZE);
        int off = inumber % 128;

        if (read_block(mounted_disk, inumber / 128 + sb->inode_block_idx, in))
        {
            printf("{SFS} -- [ERROR] Failed to read inode -- Failed to get inumber\n");
            return -1;
        }

        if (in[off].valid == 0)
        {
            printf("{SFS} -- [ERROR] Invalid inode -- Failed to get inumber\n");
            return -1;
        }

        // Read the data block
        dir_entry *dir = (dir_entry *)malloc(in[off].size);
        int len = read_i(inumber, (char *)dir, in[off].size, 0);
        free(in);

        if (len < 0)
        {
            printf("{SFS} -- [ERROR] Failed to read data block -- Failed to get inumber\n");
            return -1;
        }

        int num_entries = len / sizeof(dir_entry);
        for (int j = 0; j < num_entries; j++)
        {
            if (dir[j].inode_idx != 0)
            {
                recursive_remove(dir[j].inode_idx, dir[j].type);
            }
        }

        free(dir);
    }

    if (remove_file(inumber) < 0)
    {
        printf("{SFS} -- [ERROR] Failed to remove file -- Failed to remove file\n");
        return -1;
    }
    free_inode(inumber);
    return 0;
}

int validate_path(char **dirs, int nun)
{
    for (int x = 0; x < nun; x++)
    {
        if (strlen(dirs[x]) == 0)
        {
            return 0;
        }
    }
    return 1;
}

void free_data_block(int block_idx)
{
    if (block_idx < 0)
    {
        printf("{SFS} -- [ERROR] Invalid block index -- Failed to free data block\n");
        return;
    }

    if (block_idx >= sb->data_blocks)
    {
        printf("{SFS} -- [ERROR] Invalid block index -- Failed to free data block\n");
        return;
    }

    int bit_block_idx = block_idx / (8 * BLOCKSIZE);
    int bit_offset = block_idx % (8 * BLOCKSIZE);
    bitset *bit_block = (bitset *)malloc(BLOCKSIZE);
    read_block(mounted_disk, bit_block_idx + sb->data_block_bitmap_idx, bit_block);
    unset(bit_block, bit_offset);
    write_block(mounted_disk, bit_block_idx + sb->data_block_bitmap_idx, bit_block);
}

void free_inode(int inumber)
{
    if (inumber < 0)
    {
        printf("{SFS} -- [ERROR] Invalid block index -- Failed to free data block\n");
        return;
    }

    if (inumber >= sb->inodes)
    {
        printf("{SFS} -- [ERROR] Invalid block index -- Failed to free data block\n");
        return;
    }

    int bit_inumber = inumber / (8 * BLOCKSIZE);
    int bit_offset = inumber % (8 * BLOCKSIZE);
    bitset *bit_block = (bitset *)malloc(BLOCKSIZE);
    read_block(mounted_disk, bit_inumber + sb->inode_bitmap_block_idx, bit_block);
    unset(bit_block, bit_offset);
    write_block(mounted_disk, bit_inumber + sb->inode_bitmap_block_idx, bit_block);
}