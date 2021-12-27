#include "disk.h"
#include "sfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_dir(dir_entry *info)
{
	printf("File name  : %s\n", info->name);
	printf("File type  : %d\n", info->type);
	printf("File valid : %d\n", info->valid);
	printf("Name length: %d\n", info->name_len);
	printf("File inode : %d\n", info->inode_idx);
}

int print_err(disk *diskptr, int test)
{
	printf("TEST %d: \033[1;31mFAILED\033[0m. TERMINATING..\n", test);
	if (diskptr)
		return free_disk(diskptr);
	exit(0);
}

void print_pass(int test)
{
	printf("TEST %d: \033[1;32mPASSED\033[0m\n", test);
}

void print_str(char *x, int len)
{
	for (int i = 0; i < len; i++)
		printf("%c", x[i]);
	printf("\n");
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

void print_data_block(int block, disk *diskptr)
{
	super_block *sb = (super_block *)malloc(sizeof(super_block));
	read_block(diskptr, 0, sb);

	int data_block_start = sb->data_block_idx;
	char *data_block = (char *)malloc(BLOCKSIZE);
	if (read_block(diskptr, data_block_start + block, data_block) == -1)
		return;
	printf("\n\n [DATABLOCK] \n %s \n\n", data_block);
}

int main()
{
	int test_no = 1;
	printf("TEST %d : ------------------------CREATING DISK-----------------------------\n", test_no);
	int DISKSIZE = 100 * 4096 + 24; //creating disk of size 100 BLOCKS + 24 bytes

	disk *test = create_disk(DISKSIZE);
	print_pass(test_no);
	test_no++;

	printf("TEST %d : ----------------------TESTING WRITE BLOCK-------------------------\n", test_no);
	char str1[BLOCKSIZE];
	char str2[7] = "testing";
	for (int i = 0; i < 585; i++)
		memcpy(str1 + i * 7, str2, 7);
	str1[BLOCKSIZE - 1] = '\0';
	if (write_block(test, 0, (void *)str1) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d : -----------------------TESTING READ BLOCK-------------------------\n", test_no);
	char str3[BLOCKSIZE];
	if (read_block(test, 0, (void *)str3) < 0 || strcmp(str3, str1) != 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d : -------------------------FORMATTING DISK--------------------------\n", test_no);
	if (format(test) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d : -------------------CREATING FILE BEFORE MOUNTING------------------\n", test_no);
	int inumber = create_file(); //should return -1
	if (inumber < 0)
		print_pass(test_no);
	else
		return print_err(test, test_no);
	test_no++;

	printf("TEST %d : -------------------------MOUNTING DISK----------------------------\n", test_no);
	if (mount(test) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d : -------------------------CREATING FILE----------------------------\n", test_no);
	inumber = create_file();
	if (inumber < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d : -------------------------CHECKING STAT----------------------------\n", test_no);
	if (stat(inumber) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	char str4[] = "India fantastic!!";
	printf("TEST %d : ------------------------TESTING WRITE_I---------------------------\n", test_no);
	if (write_i(inumber, str4, 17, 0) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	char str5[17];

	printf("TEST %d: ------------------------ TESTING READ_I---------------------------\n", test_no);
	if (read_i(inumber, str5, 17, 0) < 0 || compare(str4, str5, 17) != 0)
		return print_err(test, test_no);
	print_pass(test_no);
	printf("Read: "); //should be "India fantastic!!"
	print_str(str5, 17);
	test_no++;

	char str6[3];

	printf("TEST %d: ------------------------ TESTING READ_I---------------------------\n", test_no);
	if (read_i(inumber, str6, 3, 1) < 0 || compare(str4 + 1, str6, 3) != 0)
		return print_err(test, test_no);
	print_pass(test_no);
	printf("Read : "); //should be "ndi"
	print_str(str6, 3);
	test_no++;

	printf("TEST %d: ------------------------TESTING WRITE_I---------------------------\n", test_no);
	if (write_i(inumber, str4 + 2, 2, 5) < 0) //write "di" in " f"
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: ------------------------ TESTING READ_I---------------------------\n", test_no);
	if (read_i(inumber, str5, 17, 0) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	printf("Read : "); //should be "Indiadiantastic!!"
	print_str(str5, 17);
	test_no++;

	char str7[BLOCKSIZE * 6]; //large data
	str1[4095] = 'x';		  //str1 is now "testingtesting.....testingx"
	for (int i = 0; i < 6; i++)
		memcpy(str7 + i * BLOCKSIZE, str1, 4096);

	printf("TEST %d: -------------------------CREATING FILE----------------------------\n", test_no);
	int inumber2 = create_file();
	if (inumber2 < 0)
		return print_err(test, test_no);
	print_pass(test_no);

	printf("TEST %d: -------------------------CHECKING STAT----------------------------\n", test_no);
	if (stat(inumber2) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: ------------------------TESTING WRITE_I---------------------------\n", test_no);
	if (write_i(inumber2, str7, 6 * BLOCKSIZE, 0) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	char str8[6 * BLOCKSIZE];
	printf("TEST %d: ------------------------ TESTING READ_I---------------------------\n", test_no);
	if (read_i(inumber2, str8, 6 * BLOCKSIZE, 0) < 0 || compare(str8, str7, 6 * BLOCKSIZE) != 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: -------------------------CHECKING STAT----------------------------\n", test_no);
	if (stat(inumber2) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	char str9[6] = "canada";

	char str10[6];
	printf("TEST %d: ------------------------TESTING WRITE_I---------------------------\n", test_no);
	if (write_i(inumber2, str9, 6, 6 * 4096 - 3) < 0) //writing canada from 3rd last byte of previous data written in inode 2
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("\n\n[DEBUG] : str9 = %s\n\n", str9);

	printf("TEST %d: ------------------------ TESTING READ_I---------------------------\n", test_no);
	if (read_i(inumber2, str10, 6, 6 * 4096 - 3) < 0 || compare(str9, str10, 6) != 0)
		return print_err(test, test_no);
	print_pass(test_no);
	printf("Read : "); //should be "canada"
	print_str(str10, 6);
	test_no++;

	printf("TEST %d: -------------------------CHECKING STAT----------------------------\n", test_no);
	if (stat(inumber2) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: -------------------------CREATING FILE----------------------------\n", test_no);
	int inumber3 = create_file();
	if (inumber3 < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	char str11[7 * BLOCKSIZE + 1];
	for (int i = 0; i < 7; i++)
		memcpy(str11 + i * BLOCKSIZE, str1, BLOCKSIZE);
	str11[7 * BLOCKSIZE] = 'y';

	printf("TEST %d: ------------------------TESTING WRITE_I---------------------------\n", test_no);
	if (write_i(inumber3, str11, 7 * BLOCKSIZE + 1, 0) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	char str12[7 * BLOCKSIZE + 1];
	printf("TEST %d: ------------------------ TESTING READ_I---------------------------\n", test_no);
	if (read_i(inumber3, str12, 7 * BLOCKSIZE + 1, 0) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: -------------------------CHECKING STAT----------------------------\n", test_no);
	if (stat(inumber3) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: ------------------------TRUNCATING FILE---------------------------\n", test_no);
	if (fit_to_size(inumber3, 5) < 0) //reduce size to 5 bytes
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: -------------------------CHECKING STAT----------------------------\n", test_no);
	if (stat(inumber3) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	char str13[6];
	printf("TEST %d: ------------------------ TESTING READ_I---------------------------\n", test_no);
	if (read_i(inumber3, str13, 6, 0) == 5 && compare(str13, str11, 5) == 0)
		print_pass(test_no);
	else
		return print_err(test, test_no);
	test_no++;

	printf("TEST %d: -------------------------REMOVING FILE----------------------------\n", test_no);
	if (remove_file(inumber) < 0) //inode 1
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: -------------------------REMOVING FILE----------------------------\n", test_no);
	if (remove_file(inumber2) < 0) //inode 2
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: -------------------------REMOVING FILE----------------------------\n", test_no);
	if (remove_file(inumber3) < 0) //inode 3
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: ----------------------REMOVING WRONG INUMBER----------------------\n", test_no);
	if (remove_file(10) < 0)
		print_pass(test_no);
	else
		return print_err(test, test_no);
	test_no++;

	printf("TEST %d: -------------------------CHECKING STAT----------------------------\n", test_no);
	if (stat(inumber) < 0) //should print all fileds 0 as file has been removed
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: -------------------------CHECKING STAT----------------------------\n", test_no);
	if (stat(inumber2) < 0) //should print all fileds 0 as file has been removed
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: -------------------------CHECKING STAT----------------------------\n", test_no);
	if (stat(inumber3) < 0) //should print all fileds 0 as file has been removed
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: -------------------------CREATING FILE----------------------------\n", test_no);
	int inumber4 = create_file();
	if (inumber4 < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	strcpy(str13, "india");
	printf("TEST %d: ------------------TESTING WRITE_I BEYOND SIZE---------------------\n", test_no);
	if (write_i(inumber4, str13, 5, 5) != 10) //should write 10 bytes starting from offset 0
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	char str14[6];
	printf("TEST %d: ------------------------ TESTING READ_I---------------------------\n", test_no);
	if (read_i(inumber4, str14, 5, 5) < 0 || compare(str14, str13, 5) != 0) //should read "india"
		return print_err(test, test_no);
	print_pass(test_no);
	printf("Read : ");
	print_str(str14, 5);
	test_no++;

	printf("TEST %d: -------------------------CHECKING STAT----------------------------\n", test_no);
	if (stat(inumber4) < 0) //size should be 10 bytes
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: -------------------------REMOVING FILE----------------------------\n", test_no);
	if (remove_file(inumber4) < 0) //inode 3
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	dir_entry info;

	char dir1[] = "dir1"; //NOTE full path may/may not start/end with /
	printf("TEST %d: -----------------------CREATING DIRECTORY-------------------------\n", test_no);
	inumber = create_dir(dir1);
	if (inumber < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: -------------------------CHECKING STAT----------------------------\n", test_no);
	if (stat(0) < 0) //check stats of root, size should be sizeof(dir_entry)=256bytes
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	char dir[] = "/";
	printf("TEST %d: --------------------------READING FILE----------------------------\n", test_no);
	if (read_file(dir, (char *)(&info), sizeof(dir_entry), 0) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	print_dir(&info); //should have the filename dir1
	test_no++;

	printf("TEST %d: -----------------------REMOVING DIRECTORY-------------------------\n", test_no);
	if (remove_dir(dir1) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: -------------------------CHECKING STAT----------------------------\n", test_no);
	if (stat(0) < 0) //check stats of root, size should be 0
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	char invalid_dir[] = "/a//";
	printf("TEST %d: -------------CREATING DIRECTORY PASSING WRONG PATH-----------------\n", test_no);
	if (create_dir(invalid_dir) < 0)
		print_pass(test_no);
	else
		return print_err(test, test_no);
	test_no++;

	//create dir1/dir2/dir3, write into dir1/dir2/dir3/file1
	char dir2[] = "/dir1/dir2/";
	char dir3[] = "/dir1/dir2/dir3";

	printf("TEST %d: -----------------------CREATING DIRECTORY-------------------------\n", test_no);
	inumber = create_dir(dir1);
	if (inumber < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: -----------------------CREATING DIRECTORY-------------------------\n", test_no);
	inumber2 = create_dir(dir2);
	if (inumber2 < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: -----------------------CREATING DIRECTORY-------------------------\n", test_no);
	inumber3 = create_dir(dir3);
	if (inumber3 < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	char file1[] = "dir1/dir2/dir3/file1/";
	char str15[6 * BLOCKSIZE + 3];
	for (int i = 0; i < 6; i++)
		memcpy(str15 + i * BLOCKSIZE, str1, BLOCKSIZE);
	str15[6 * BLOCKSIZE] = 'p';
	str15[6 * BLOCKSIZE + 1] = 'q';
	str15[6 * BLOCKSIZE + 2] = 'r';

	printf("TEST %d: --------------------------WRITING FILE----------------------------\n", test_no);
	if (write_file(file1, str15, 6 * BLOCKSIZE + 3, 0) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	char str16[5];
	printf("TEST %d: --------------------------READING FILE----------------------------\n", test_no);
	if (read_file(file1, str16, 5, 6 * BLOCKSIZE - 2) < 0 && compare(str16, str15 + (6 * BLOCKSIZE - 2), 5) != 0)
		return print_err(test, test_no);
	print_pass(test_no);
	printf("Read : "); //should be "gxpqr"
	print_str(str16, 5);
	test_no++;

	printf("TEST %d: -----------------------REMOVING DIRECTORY-------------------------\n", test_no);
	if (remove_dir(dir1) < 0) //now recursive removals also has to be done of sub directories
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	//0->should be of 0 size, 1,2,3,4->should be invalid and 0 size as dir1 removed
	for (int i = 0; i < 5; i++)
	{
		printf("TEST %d: -------------------------CHECKING STAT----------------------------\n", test_no);
		if (stat(i) < 0)
			return print_err(test, test_no);
		print_pass(test_no);
		test_no++;
	}

	//create absent directories now too
	printf("TEST %d: -----------------------CREATING DIRECTORY-------------------------\n", test_no);
	if (create_dir(dir3) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	//create and write a file with path dir1/dir2/file2
	char file2[] = "/dir1/dir2/file2";
	char str17[9] = "sometext";
	printf("TEST %d: --------------------------WRITING FILE----------------------------\n", test_no);
	if (write_file(file2, str17, 8, 4094) < 0) //last 2 bytes of 1st block and 1st 6 bytes of 2nd
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	char str18[8];
	if (read_file(file2, str18, 8, 4094) < 0 || compare(str18, str17, 8) != 0)
		return print_err(test, test_no);
	print_pass(test_no);
	printf("Read : "); //should be "sometext"
	print_str(str18, 8);
	test_no++;

	printf("TEST %d: -------------------------CHECKING STAT----------------------------\n", test_no);
	if (stat(2) < 0) //dir2 must have inumber 2 on basis of the scheme of allocating 1st free inode
		return print_err(test, test_no);
	print_pass(test_no); //size shoulde be 2*sizeof(dir_entry)=512 bytes since dir3, and file2 in dir1
	test_no++;

	char non_existent[] = "/dir3/file.txt";
	printf("TEST %d: ----------------TESTING WRITE TO NON EXSITING FILE----------------\n", test_no);
	if (write_file(non_existent, str17, 8, 0) < 0)
		print_pass(test_no);
	else
		return print_err(test, test_no);
	test_no++;

	printf("TEST %d: -----------------------REMOVING DIRECTORY-------------------------\n", test_no);
	if (remove_dir(dir1) < 0)
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: ----------------------CREATING DIRECTORIES------------------------\n", test_no);
	for (int i = 0; i < 26; i++) //create 26 directories under root
	{
		char alpha[] = "/";
		char c = 'a' + i;
		strncat(alpha, &c, 1);
		if (create_dir(alpha) < 0)
			return print_err(test, test_no);
	}
	print_pass(test_no);
	test_no++;

	printf("TEST %d: -------------------------CHECKING STAT----------------------------\n", test_no);
	if (stat(0) < 0) //size of root will now be 26*sizeof(dir_entry)=26*256=6656bytes
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("TEST %d: ----------------------REMOVING DIRECTORIES------------------------\n", test_no);
	for (int i = 0; i < 16; i++)
	{
		char alpha[] = "/";
		char c = 'a' + i;
		strncat(alpha, &c, 1);
		if (remove_dir(alpha) < 0)
			return print_err(test, test_no);
	}
	print_pass(test_no);
	test_no++;

	printf("TEST %d: -------------------------CHECKING STAT----------------------------\n", test_no);
	if (stat(0) < 0) //size of root will now be 10*sizeof(dir_entry)=2560 bytes
		return print_err(test, test_no);
	print_pass(test_no);
	test_no++;

	printf("ALL TESTS PASSED!! FREEING DISK..\n");

	free_disk(test);

	return 0;
}