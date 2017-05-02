
#include "fs.h"
#include "disk.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define FS_MAGIC           0xf0f03410
#define INODES_PER_BLOCK   128
#define POINTERS_PER_INODE 5
#define POINTERS_PER_BLOCK 1024

int *freeBlockBitmap;
int *inodeBitmap;

struct fs_superblock {
	int magic;
	int nblocks;
	int ninodeblocks;
	int ninodes;
};

struct fs_inode {
	int isvalid;
	int size;
	int direct[POINTERS_PER_INODE];
	int indirect;
};

union fs_block {
	struct fs_superblock super;
	struct fs_inode inode[INODES_PER_BLOCK];
	int pointers[POINTERS_PER_BLOCK];
	char data[DISK_BLOCK_SIZE];
};

int fs_format()
{	
	int nblocks = disk_size();
	int ninodeblocks = 0;
	
	//Set number of inode blocks to 10% total blocks 
	if (nblocks % 10 == 0) ninodeblocks = nblocks/10;
	else ninodeblocks = nblocks/10 + 1;

	//Create new union with super block
	union fs_block superblock;
	superblock.super.magic = FS_MAGIC;
	superblock.super.nblocks = nblocks;
	superblock.super.ninodeblocks = ninodeblocks;
	superblock.super.ninodes = (ninodeblocks * INODES_PER_BLOCK);

	//Write the new superblock to the disk
	disk_write(0, superblock.data);

	return 1;
}

void fs_debug()
{
	union fs_block block;
	union fs_block temp;
	union fs_block tempindirect;
	
	// disk_read(54, temp.data);
	// int p;
	// for(p=0; p<5; p++){
		// printf("%d ", temp.pointers[p]);
	// }
	// printf("\n");

	disk_read(0,block.data);

	if(block.super.magic != FS_MAGIC){
		printf("magic number is invalid\n");
		return;
	}
	printf("superblock:\n");
	printf("\tmagic number is valid\n");
	printf("\t%d blocks\n",block.super.nblocks);
	printf("\t%d inode blocks\n",block.super.ninodeblocks);
	printf("\t%d inodes\n",block.super.ninodes);
	
	int i, j, k, l;
	int inodenum;
	int inodenblocks;
	for(i=1; i<=block.super.nblocks; i++){
		if(i<=block.super.ninodeblocks){
			disk_read(i, temp.data);
			for(j=0; j<INODES_PER_BLOCK; j++){
				if(temp.inode[j].isvalid == 1){
					inodenblocks = temp.inode[j].size/DISK_BLOCK_SIZE + 1;
					//printf("%d\n", inodenblocks);
					inodenum = (i-1)*INODES_PER_BLOCK+j;
					printf("inode %d:\n", inodenum);
					printf("\tsize: %d bytes\n", temp.inode[j].size);
					printf("\tdirect blocks: ");
					
					for(k=0; k<inodenblocks; k++){
						if(k<POINTERS_PER_INODE){
							printf(" %d", temp.inode[j].direct[k]);
						} else if(k==POINTERS_PER_INODE){
							printf("\n\tindirect block: %d", temp.inode[j].indirect);
							disk_read(temp.inode[j].indirect, tempindirect.data);
							printf("\n\tindirect data blocks: ");
							for(l=0; l<inodenblocks-k; l++){
								printf(" %d", tempindirect.pointers[l]);
							}
							
							break;
						}
						
						
					}
					printf("\n");
				}
			}
		}
	}
}

int fs_mount()
{
	union fs_block block;
	union fs_block temp;
	union fs_block tempindirect;
	disk_read(0, block.data);
	if(block.super.magic != FS_MAGIC){
		printf("invalid file system\n");
		return 0;
	}
	freeBlockBitmap = malloc(sizeof(int)*block.super.nblocks);
	inodeBitmap = malloc(sizeof(int)*block.super.ninodes);
	int i, j, k, l, inodenblocks, inodenum;
	for(i=0; i<block.super.ninodes; i++){
		inodeBitmap[i] = 0;
	}
	for(i=0; i<block.super.nblocks; i++){
		if(i<=block.super.ninodeblocks){
			freeBlockBitmap[i] = 1;
		} else {
			freeBlockBitmap[i] = 0;
		}
	}
	
	for(i=1; i<=block.super.ninodeblocks; i++){
		disk_read(i, temp.data);
		for(j=0; j<INODES_PER_BLOCK; j++){
			if(temp.inode[j].isvalid){
				inodenum = (i-1)*INODES_PER_BLOCK+j;
				inodeBitmap[inodenum] = 1;
				inodenblocks = temp.inode[j].size/DISK_BLOCK_SIZE + 1;				
				for(k=0; k<inodenblocks; k++){
					if(k<POINTERS_PER_INODE){
						freeBlockBitmap[temp.inode[j].direct[k]] = 1;
					} else if(k==POINTERS_PER_INODE){
						disk_read(temp.inode[j].indirect, tempindirect.data);
						for(l=0; l<inodenblocks-k; l++){
							freeBlockBitmap[tempindirect.pointers[l]] = 1;
						}
						
						break;
					}
					
					
				}
			}
		}
	}
	
	
	return 1;
}

int fs_create()
{
	return 0;
}

int fs_delete( int inumber )
{
	return 0;
}

int fs_getsize( int inumber )
{
	return -1;
}

int fs_read( int inumber, char *data, int length, int offset )
{
	return 0;
}

int fs_write( int inumber, const char *data, int length, int offset )
{
	return 0;
}
