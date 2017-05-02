
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

int IS_MOUNTED = 0;
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
	if(IS_MOUNTED == 1) return 0;

	int nblocks = disk_size();
	int ninodeblocks = 0;
	int i = 0;
	int j=0;
	union fs_block temp;

	//Set number of inode blocks to 10% total blocks 
	if (nblocks % 10 == 0) ninodeblocks = nblocks/10;
	else ninodeblocks = nblocks/10 + 1;

	//Create new union with super block
	union fs_block block;
	block.super.magic = FS_MAGIC;
	block.super.nblocks = nblocks;
	block.super.ninodeblocks = ninodeblocks;
	block.super.ninodes = (ninodeblocks * INODES_PER_BLOCK);

	//Create new bitmap
	freeBlockBitmap = malloc(sizeof(int)*block.super.nblocks);
	inodeBitmap = malloc(sizeof(int)*block.super.ninodes);

	//Write the new superblock to the disk
	disk_write(0, block.data);

	//Reset inode bitmap
	for(i=0; i<block.super.ninodes; i++){
		inodeBitmap[i] = 0;
	}

	for(i=0; i< nblocks; i++){
		freeBlockBitmap[i] = 0;
	}

	//Reset Valid Bits in Disk 
	for(i=1; i<block.super.nblocks; i++){
		if(i<=block.super.ninodeblocks){
			disk_read(i, temp.data);
			for(j=0; j<INODES_PER_BLOCK; j++){
				if(temp.inode[j].isvalid == 1){
					temp.inode[j].isvalid = 0;
				}
			}
		}
		disk_write(i,temp.data);
	}

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
	
	IS_MOUNTED = 1;
	return 1;
}

int fs_create()
{
	union fs_block block;
	disk_read(0, block.data);
	if(block.super.magic != FS_MAGIC){
		printf("invalid file system\n");
		return 0;
	}
	int i;
	int blocknum;
	for(i=1; i<block.super.ninodes; i++){
		if(inodeBitmap[i] == 0){
			blocknum = i/INODES_PER_BLOCK + 1;
			disk_read(blocknum, block.data);
			block.inode[i].isvalid = 1;
			block.inode[i].size = 0;
			inodeBitmap[i] = 1;
			disk_write(blocknum, block.data);
			return i;
		}
	}
	printf("No free inodes\n");
	return 0;
}

int fs_delete( int inumber )
{
	union fs_block block;
	union fs_block temp;
	disk_read(0, block.data);
	if(block.super.magic != FS_MAGIC){
		printf("invalid file system\n");
		return 0;
	}
	inodeBitmap[inumber] = 0;
	
	int blocknum, position, inodenblocks, i, j;
	
	blocknum = inumber/INODES_PER_BLOCK + 1;
	position = inumber - INODES_PER_BLOCK*(blocknum-2);
	disk_read(blocknum, block.data);
	block.inode[position].isvalid = 0;
	inodenblocks = block.inode[position].size/DISK_BLOCK_SIZE + 1;
	for(i=0; i<inodenblocks; i++){
		if(i<POINTERS_PER_INODE){
			freeBlockBitmap[i]=0;
		} else if(i == POINTERS_PER_INODE){
			disk_read(block.inode[position].indirect, temp.data);
			freeBlockBitmap[block.inode[position].indirect] = 0;
			for(j=0; j<inodenblocks-i; j++){
				freeBlockBitmap[temp.pointers[j]] = 0;
			}
			return 1;
		}
	}
	
	return 0;
}

int fs_getsize( int inumber )
{
	int size = 0;
	
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
