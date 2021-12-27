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
File Name       disc.c
------------------------------------------
*/

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "disk.h"

disk *create_disk(int nbytes)
{
    disk *diskptr = (disk *)malloc(sizeof(disk));

    diskptr->size = nbytes;
    diskptr->reads = 0;
    diskptr->writes = 0;
    diskptr->blocks = (nbytes - STATSSIZE) / BLOCKSIZE;

    diskptr->block_arr = (char **)malloc(sizeof(char *) * diskptr->blocks);
    if (diskptr->block_arr == NULL)
    {
        printf("{DISK} -- [ERROR]: malloc failed ... No more space left to allocate\n");
        return NULL;
    }

    for (int i = 0; i < diskptr->blocks; i++)
    {
        diskptr->block_arr[i] = (char *)malloc(sizeof(char) * BLOCKSIZE);
        memset(diskptr->block_arr[i], 0, BLOCKSIZE);
        if (diskptr->block_arr[i] == NULL)
        {
            printf("{DISK} -- [ERROR]: malloc failed ... No more space left to allocate\n");
            return NULL;
        }
    }

    return diskptr;
}

int read_block(disk *diskptr, int blocknr, void *block_data)
{
    if (blocknr < 0 || blocknr >= diskptr->blocks)
    {
        printf("{DISK} -- [ERROR - READ]: block number [%d] out of range\n", blocknr);
        return -1;
    }

    if (block_data == NULL)
    {
        printf("{DISK} -- [ERROR]: block data is NULL\n");
        return -1;
    }

    memcpy(block_data, diskptr->block_arr[blocknr], BLOCKSIZE);
    diskptr->reads++;
    return 0;
}

int write_block(disk *diskptr, int blocknr, void *block_data)
{
    if (blocknr < 0 || blocknr >= diskptr->blocks)
    {
        printf("{DISK} -- [ERROR - WRITE]: block number [%d] out of range\n", blocknr);
        return -1;
    }

    memcpy(diskptr->block_arr[blocknr], block_data, BLOCKSIZE);
    diskptr->writes++;
    return 0;
}

int free_disk(disk *diskptr)
{
    for (int i = 0; i < diskptr->blocks; i++)
    {
        free(diskptr->block_arr[i]);
    }
    free(diskptr->block_arr);
    free(diskptr);
    return 0;
}