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
File Name       main.c
------------------------------------------
*/

#include "disk.h"
#include "sfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

disk *diskptr;

void print_dir_entry(int inumber)
{
	if (inumber < 0)
	{
		printf("{SFS} -- [ERROR] Invalid inumber -- Failed to print directory\n");
		return;
	}

	inode *in = (inode *)malloc(BLOCKSIZE);
	int off = inumber % 128;

	super_block *sb = (super_block *)malloc(BLOCKSIZE);
	read_block(diskptr, 0, sb);
	read_block(diskptr, inumber / 128 + sb->inode_block_idx, in);

	dir_entry *dir = (dir_entry *)malloc(in[off].size);
	int len = read_i(inumber, (char *)dir, in[off].size, 0);
	free(in);

	if (len < 0)
	{
		printf("{SFS} -- [ERROR] Failed to read data block -- Failed to print directory\n");
		return;
	}

	int num_entries = len / sizeof(dir_entry);

	printf("\n+---------------------------------------+\n");
	printf("|               INODE %2d                |\n", inumber);
	printf("+---------------------------------------+\n");
	printf("|    NAME   |    TYPE   | VALID | INODE |\n");
	printf("+---------------------------------------+\n");
	for (int i = 0; i < num_entries; i++)
	{
		if (dir[i].valid == 1)
		{
			printf("| %9s | %9s | %5d | %5d |\n", dir[i].name, dir[i].type == 1 ? "Directory" : "File", dir[i].valid, dir[i].inode_idx);
		}
	}
	printf("+---------------------------------------+\n\n");
}

int print_err(disk *diskptr, char *testname, int test)
{
	printf("TEST %2d: \033[1;31mFAILED\033[0m %s\n", test, testname);
	printf("+-------------------------------------+\n");
	printf("|\033[1;31m          Terminating Tests\033[0m          |\n");
	printf("+-------------------------------------+\n\n");
	if (diskptr)
		free_disk(diskptr);
	exit(0);
}

void print_pass(int test, char *testname)
{
	printf("TEST %2d: \033[1;32mPASSED\033[0m %s\n", test, testname);
}

int compare(char *x, char *y, int len)
{
	if (!x || !y)
		return -1;
	for (int i = 0; i < len; i++)
	{
		if (x[i] != y[i])
			return -1;
	}
	return 0;
}

void run_tests()
{
	int disk_size = 100 * BLOCKSIZE + 24;
	diskptr = create_disk(disk_size);
	printf("\033[1;35m\n\
	+--------------------------+\n\
	|      Creating disk       |\n\
	+--------------------------+\n\
	|  size (bytes): %8d  |\n\
	|  blocks: %14d  |\n\
	+--------------------------+\033[0m\n\n",
		   disk_size, disk_size / BLOCKSIZE);

	char s_sm[] = "Hello World";
	char dir[] = "/home/18CS30051/";
	char file[] = "/home/18CS30051/test.txt";
	char file_lg[] = "/home/18CS30051/file.txt";

	char s_lg[strlen(s_sm) * BLOCKSIZE];
	for (int i = 0; i < strlen(s_sm) * BLOCKSIZE; i += strlen(s_sm))
		strcpy(s_lg + i, s_sm);

	char *tests = (char *)malloc(sizeof(char) * 100);

	printf("\033[1;34m Runnning Tests\033[0m\n\n");

	int inumber = -1, testcase = 1;

	// ------------------------- TEST SUITE 1 ------------------------- //
	printf("\033[1;35m\n Test Suite 1: Part A\033[0m\n\n");
	sprintf(tests, "create_disk %d bytes (%d blocks + %d bytes)", disk_size, disk_size / BLOCKSIZE, disk_size % BLOCKSIZE);
	if (diskptr == NULL)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	sprintf(tests, "write_block %d bytes", BLOCKSIZE);
	if (write_block(diskptr, 1, s_sm) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	sprintf(tests, "read_block %d bytes", BLOCKSIZE);
	char *buf = (char *)malloc(BLOCKSIZE);
	if (read_block(diskptr, 1, buf) < 0)
		print_err(diskptr, tests, testcase);
	else if (compare(buf, s_sm, BLOCKSIZE) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	free(buf);
	testcase++;

	// ------------------------- TEST SUITE 2 ------------------------- //
	printf("\033[1;35m\n Test Suite 2: Part B\033[0m\n\n");
	sprintf(tests, "format_disk");
	if (format(diskptr) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);

	sprintf(tests, "mount_disk");
	if (mount(diskptr) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	sprintf(tests, "create_file ");
	if ((inumber = create_file(diskptr, file)) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	sprintf(tests, "write_i %ld bytes", strlen(s_sm));
	if (write_i(inumber, s_sm, strlen(s_sm), 0) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	sprintf(tests, "read_i %d bytes", 5);
	buf = (char *)malloc(5);
	if (read_i(inumber, buf, 5, 0) < 0)
		print_err(diskptr, tests, testcase);
	else if (compare(buf, s_sm, 5) != 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	free(buf);
	testcase++;

	sprintf(tests, "stats before truncate");
	if (stat(inumber) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	sprintf(tests, "truncate_file to %d bytes", 5);
	if (fit_to_size(inumber, 5) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	sprintf(tests, "stats after truncate");
	if (stat(inumber) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	sprintf(tests, "write_i %ld bytes", strlen(s_lg));
	if (write_i(inumber, s_lg, strlen(s_lg), 0) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	sprintf(tests, "stats for large input");
	if (stat(inumber) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	sprintf(tests, "read_i %ld bytes", strlen(s_lg) / 2);
	buf = (char *)malloc(strlen(s_lg) / 2);
	if (read_i(inumber, buf, strlen(s_lg) / 2, strlen(s_lg) / 2) < 0)
		print_err(diskptr, tests, testcase);
	else if (compare(buf, s_lg + strlen(s_lg) / 2, strlen(s_lg) / 2) != 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	free(buf);
	testcase++;

	sprintf(tests, "write_i %ld bytes at offset %d", strlen(s_sm), 20);
	if (write_i(inumber, s_sm, strlen(s_sm), 20) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);

	sprintf(tests, "read_i %d bytes at offset %d", 5, 20);
	buf = (char *)malloc(5);
	if (read_i(inumber, buf, 5, 20) < 0)
		print_err(diskptr, tests, testcase);
	else if (compare(buf, s_sm, 5) != 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);

	stat(inumber);

	sprintf(tests, "remove_file");
	if (remove_file(inumber) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	sprintf(tests, "stats after removing");
	if (stat(inumber) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	// ------------------------- TEST SUITE 3 ------------------------- //
	printf("\033[1;35m\n Test Suite 3: Part C\033[0m\n\n");
	sprintf(tests, "create_dir %s", dir);
	if (create_dir(dir) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;
	print_dir_entry(0);
	print_dir_entry(1);

	sprintf(tests, "remove_dir %s", dir);
	if (remove_dir(dir) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	char dir_multi[] = "/.";
	sprintf(tests, "create_dir 26 directories");
	int error = 0;
	for (int i = 0; i < 26; i++)
	{
		dir_multi[1] = 'a' + i;
		error = error || (create_dir(dir_multi) < 0);
	}
	if (error)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;
	print_dir_entry(0);

	sprintf(tests, "remove_dir 26 directories");
	error = 0;
	for (int i = 0; i < 26; i++)
	{
		dir_multi[1] = 'a' + i;
		error = error || (remove_dir(dir_multi) < 0);
	}
	if (error)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	sprintf(tests, "write_file %s %ld (small input) bytes", file, strlen(s_sm));
	if (create_dir(dir) && write_file(file, s_sm, strlen(s_sm), 0) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	sprintf(tests, "read_file  %s  %d (small input) bytes", file, 5);
	buf = (char *)malloc(5);
	if (read_file(file, buf, 5, 0) < 0)
		print_err(diskptr, tests, testcase);
	else if (compare(buf, s_sm, 5) != 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	free(buf);
	testcase++;

	int sz = 1000;
	sprintf(tests, "write_file  %s (large input) %d bytes at offset %d", file_lg, sz, 10);
	if (write_file(file_lg, s_lg, sz, 10) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	sprintf(tests, "read_file   %s (large input) %d bytes from offset %d", file_lg, sz, 10);
	buf = (char *)malloc(sz);
	if (read_file(file_lg, buf, sz, 10) < 0)
		print_err(diskptr, tests, testcase);
	else if (compare(buf, s_lg, sz) != 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	print_dir_entry(0);
	print_dir_entry(1);
	print_dir_entry(2);
	
	sprintf(tests, "remove_file %s (small file)", file);
	if (remove_dir(file) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;

	sprintf(tests, "remove_file %s (large file)", file_lg);
	if (remove_dir(file_lg) < 0)
		print_err(diskptr, tests, testcase);
	else
		print_pass(testcase, tests);
	testcase++;


	printf("\n+-------------------------------------+\n");
	printf("|\033[1;34m    All tests passed successfully\033[0m    |\n");
	printf("|\033[1;34m           Freeing disk\033[0m              |\n");
	printf("+-------------------------------------+\n\n");
	free_disk(diskptr);
}

int main()
{
	printf("\n\033[1;34m\n\
+--------------------------------------------+\n\
| Assignment 3: Simple File System           |\n\
+--------------------------------------------+\n\
| subject:        Advanced Operating Systems |\n\
| year:           2021 - 2022                |\n\
+--------------------------------------------+\n\
| Name:           Debajyoti Dasgupta         |\n\
| Roll No:        18CS30051                  |\n\
+--------------------------------------------+\n\
| Kernel Version: 5.11.0-37-generic          |\n\
| System:         Ubuntu 20.04.3 LTS         |\n\
+--------------------------------------------+\n\
|              TESTING SUITE                 |\n\
+--------------------------------------------+\n\
\033[0m\n");

	run_tests();
	return 0;
}