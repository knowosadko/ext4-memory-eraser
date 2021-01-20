#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>
#include <inttypes.h>

//superblock data OFFSETS
#define INODE_SIZE		 	0x58
#define LOG_BLOCK_SIZE			0x18
#define BLOCK_GROUP_NR			0x5a
// Block group descriptors OFFSETS
#define BG_INODE_TABLE_LO		0x8
#define BG_INODE_TABLE_HI		0x28
// inode table OFFSETS
#define BG_INODE_TABLE_LO		0x8
#define BG_INODE_TABLE_HI		0x28
// inode  OFFSETS
#define I_MODE				0x0
#define I_LINKS_COUNT			0x1A
#define I_BLOCK 			0x28 
#define I_FLAGS			0x20
#define I_SIZE_LO			0x4
// direcotry entry OFFSETS
#define NAME_LEN			0x6
#define NAME 				0x8
#define REC_LEN			0x4
#define INODE				0x0

// MY GOOGLE DOC: https://docs.google.com/document/d/1L58vlnUqYJtf_ECLBrc7ONPqz2shatI_Z6nJxgzWjgE/edit?usp=sharing


int recursion;//currently not used, recursion not implemented
		
struct Metadata{
	int16_t inode_size;
	int32_t block_size;
	int16_t block_group_number;
	int32_t bg_inode_table_lo;
	int32_t bg_inode_table_hi;
};

struct File{
	int number_of_blocks;
	int inode_number;
};

struct Metadata read_metadata(FILE *fp)
{
	int fd = fileno(fp);
	struct Metadata meta;
	int8_t* buffer = malloc(4*sizeof(int8_t));
	size_t ret = pread(fd, buffer, 4, 1024 + INODE_SIZE);
	meta.inode_size = *((int16_t*)buffer);
  	ret = pread(fd, buffer, 4, 1024 + BLOCK_GROUP_NR);
	meta.block_group_number = *((int16_t*)buffer);
	ret = pread(fd, buffer, 4, 1024 + LOG_BLOCK_SIZE);
	int log = *((int32_t*)buffer);
	int bs = 1;
	for(int i=0;i<10+log ;i++)
		bs *= 2;
	meta.block_size = bs;
	ret = pread(fd, buffer, 4, meta.block_size + BG_INODE_TABLE_LO );
	meta.bg_inode_table_lo = *((int32_t*)buffer);
	ret = pread(fd, buffer, 4, meta.block_size + BG_INODE_TABLE_HI );
	meta.bg_inode_table_hi = *((int32_t*)buffer);	
	free(buffer);
	return meta;	
}

int splitPath(char* path, char** arr)
{
	// split path by '/' into string array
	char* temp = path;
	int parts=0;
	while(*temp != '\0')
	{
		if(*temp == '/')
			parts++;	
		temp++;
	}

	temp = path;
	temp++;
	for(int i=0; i< parts ;i++)
	{
		arr[i] = malloc(100);
		int j=0;
		while(*temp != '/' && *temp != '\0')
		{
			arr[i][j] = *temp;
			j++;
			temp++;
		}
		temp++;
	}

	return parts;
}


/* TODO */
void construct_command(int inode_number, char* command)
{
	char command1[100] = "sudo debugfs -R \"stat <";
	char command3[100] = ">\" /dev/loop35p1 | cat | grep -A 1 \"EXTENTS:\" > temp";
	char command2[50];
	sprintf(command2, "%d",inode_number);
	strcat(command,command1);
	strcat(command, command2);
	strcat(command,command3);
}
// to do: implement unlinking, and do not use bash command unlink 
void bash_unlink(char* path)
{
	char command[200] = "unlink /media/konrad/ext4_exp"; //ugly	
	strcat(command, path);
	printf("Command: %s \n",command);
	system(command);	
}

void my_unlink()
{

}
// swap parse block for tree extent search
int parse_blocks(char* str,int* blocks)
{
	int index = 0;
	char* tem = str; 
	if(*str == '\0')
		return 0;
	while(*tem != ':')
		tem++;
	tem++;	
	while(*tem != ':')
		tem++;
	tem++;
	char* number1 = malloc(20);
	char* number2 = malloc(20);
	int i1= 0;
	int i2 = 0;
	while(*tem != '\0')
	{
		i1 =0;
		i2 = 0;
		while(*tem >='0' && *tem <='9')
		{
			number1[i1] = *tem;
			i1++;
			tem++;
		}
		if(*tem == ',')
		{
			number1[i1] = '\0';
			tem++;
		}
		if(*tem == '-')
		{
			tem++;
			while(*tem <= '9' && *tem >= '0')
			{
				number2[i2] = *tem;
				i2++;
				tem++;
			}
			number2[i2] = '\0';
			tem++;
		}
		int block_num1 = atoi(number1);
		if(i1 > 0 && i2 > 0)
		{
			int block_num2 = atoi(number2);
			for(int i =block_num1; i<=block_num2+1; i++ )
			{
				blocks[index] = i;
				index++;
			}	
		}
		else if(i1>0)
		{
			blocks[index] = block_num1;
			index++;
		}
		tem++;	
	}
	return index-1;		
}
	int zeroDirectoryEntry(int address, struct Metadata meta)// needed for unlinking
	{

	}
/*TODO*/


struct File findBlocks(char** names, int parts ,int* blocks, FILE* fp, struct Metadata meta )
{
	struct File data;
	int next_inode = 2; // first inode is 2 because we iterate through dirs from "."
	int8_t* buffer = malloc(4*sizeof(int8_t));
	int fd = fileno(fp);
	int mode = 0;
	for(int i =0; i < parts; i++){//for iterates through {parent_folder, son_folder, grandson_folder ...} from "/parent_folder/son_folder/grandson_folder"
		//inode metadata reading
		size_t ret = pread(fd, buffer, 2, meta.bg_inode_table_lo*meta.block_size + (next_inode-1)*meta.inode_size + I_MODE );
		mode = *((int*) buffer);
		char* mode_name = malloc(20);
		ret = pread(fd, buffer, 2, meta.bg_inode_table_lo*meta.block_size + (next_inode-1)*meta.inode_size + I_LINKS_COUNT);
		int links_count = *((int*) buffer);	
		int8_t* block_buffer = malloc(60*sizeof(int8_t));
		ret = pread(fd, block_buffer, 60, meta.bg_inode_table_lo*meta.block_size + (next_inode-1)*meta.inode_size + I_BLOCK);
		int dir_entry_address = *((int*) (block_buffer+20));
		ret = pread(fd, buffer, 4, meta.bg_inode_table_lo*meta.block_size + (next_inode-1)*meta.inode_size + I_FLAGS);
		int flags = *((int*) buffer);
		
		
		// first inode dir entry
		ret = pread(fd, buffer, 4, dir_entry_address*meta.block_size + INODE);
		next_inode = *((int*) buffer);
		//record length
		ret = pread(fd, buffer, 2, dir_entry_address*meta.block_size + REC_LEN );
		int entry_length = *((int16_t*) buffer);
		//name length
		ret = pread(fd, buffer, 1, dir_entry_address*meta.block_size + NAME_LEN);
		int name_len = *((int8_t*) buffer);
		//entry name (file name)
		char* name = malloc(name_len+1);
		name[name_len] = '\0';
		ret = pread(fd, name, name_len, dir_entry_address*meta.block_size + NAME );	
		// getting type element
		mode = mode / 0x1000;
		if(mode == 0x4)
		{
			mode_name = "Directory";	
		}
		else if(mode == 0x8)
		{
			mode_name = "Regular File";
		}
		else if(mode == 0xA)
		{
			mode_name = "Symbolic link";
		}
		else
			mode_name = "Other";
		//debug print	
		printf("\nCurrent inode: %d\n",next_inode);
		printf("Type: %s \n",mode_name);
		printf("Links count: %d \n",links_count);
		printf("Flags: %d\n",flags);
		printf("Entry address: %d \n\n",dir_entry_address);
		//debug print
		int de_number = 0;
		printf("Directory entries:\n");
		int entry_offset =0;
		while(next_inode != 0)// while next enty has non-zero inode field
		{
			printf("\n\tName: %s \n", name);
			printf("\tName len: %d \n",name_len);
			printf("\tInode: %d \n", next_inode);
			if(strcmp(name,*names)==0)// comapring name string 
			{
				printf("\n\n\t==== FOUND IT ====\n");
				printf("\tFound name: %s\n", name);
				names++;
				break;
			}
			ret = pread(fd, buffer, 4, dir_entry_address*meta.block_size + INODE+entry_offset);
			next_inode = *((int*) buffer);
			ret = pread(fd, buffer, 1, dir_entry_address*meta.block_size+ entry_offset + NAME_LEN);
			name_len = *((int16_t*) buffer);
			name = malloc(name_len+1);
			name[name_len] = '\0';
			ret = pread(fd, name, name_len, dir_entry_address*meta.block_size +entry_offset + NAME);
			ret = pread(fd, buffer, 2, dir_entry_address*meta.block_size +entry_offset + REC_LEN );
			entry_length = *((int16_t*) buffer);
			// offset needed to jump to next entry
			entry_offset += entry_length;
			de_number++;
		}
		printf("\tFound inode: %d \n", next_inode);
		
	}
	// Getting basic info about found inode
	size_t ret = pread(fd, buffer, 4, meta.bg_inode_table_lo*meta.block_size + (next_inode-1)*meta.inode_size + I_FLAGS);
	int flags = *((int*) buffer);
	ret = pread(fd, buffer, 2, meta.bg_inode_table_lo*meta.block_size + (next_inode-1)*meta.inode_size + I_MODE );
	mode = *((int*) buffer);
	ret = pread(fd, buffer, 4, meta.bg_inode_table_lo*meta.block_size + (next_inode-1)*meta.inode_size + I_SIZE_LO);
	int filesize = *((int*) buffer);
	// Below get node extents using bash
	char* command = malloc(1000);
	construct_command(next_inode,command);
	int status = system(command);
	char* in = malloc(1000);
	for(int i=0;i<1000;i++)
	{
		in[i] = '\0';
	}
	int c;
	FILE *file;
	file = fopen("temp", "r");
	size_t nread;
	if (file) {
    		while ((nread = fread(in, 1, 1000, file)) > 0)
        		fwrite(in, 1, nread, stdout);
	}
	int number_of_blocks = parse_blocks(in,blocks);
	for(int i=0;i<number_of_blocks;i++)
	{
		printf("%d\n",blocks[i]);
	}
	fclose(file);
	data.number_of_blocks = number_of_blocks;
	data.inode_number = next_inode;
	return data;
}

// Zero file data blocks
int zeroBlocks(int* blocks, struct Metadata meta,int number_of_blocks ,FILE* file)
{
    int fd = fileno(file);	
    int8_t *wbuffer = malloc(meta.block_size);
    for(int i=0;i<meta.block_size;i++)
    {
    	wbuffer[i] = (int8_t) 0;
    }
  
    for(int i=0;i<number_of_blocks;i++)
    {
	int group_offseet = 0;    
    	int wret = pwrite(fd, wbuffer, meta.block_size,  blocks[i]*meta.block_size);  	
    }
    free(wbuffer);
}
// zero node, not working properly BUG
int zeroInode(int inode_number, struct Metadata meta, FILE* file)
{
    int fd = fileno(file);	
    int8_t *wbuffer = malloc(60);
    for(int i=0;i<60;i++)
    {
    	wbuffer[i] = (int8_t) 0;
    }
    int wret = pwrite(fd, wbuffer, 60,  meta.bg_inode_table_lo + I_BLOCK + meta.inode_size*(inode_number-1));  
    
}


extern int errno;

int main(int argc, char **argv)
{


	char* in = malloc(200);
	
	printf("Options: %s\n",argv[0]);
	if(argc == 1)
	{
		printf("No arguments where given.\n");
		return 1;		
	}else if(argc == 2)
	{
		if(strcmp(argv[1],"-h")==0)
		{
			printf("++++====ERASER====++++\n");
			printf("Program for secure data erasing.\n");
			printf("It overwrites data with zeros.\n");
			printf("Options:\n");
			printf("-p filepath : path specifies file to erase.\n");
			printf("-r : recursion earasing (NOT IMPLEMENTED) \n");
			printf("Example: sudo ./earser.out -p /folder1/file.txt \n");
			return 0;
		}
	}
	else if(argc == 3)
	{
		if(strcmp(argv[1],"-p")==0){
			strcpy(in, argv[2]);
		}	
		else{ 
			printf("Wrong arguments");
			}			
	}
	else if(argc == 3)
	{
		if(strcmp(argv[1],"-p")==0)
			strcpy(in, argv[2]);
		else{ 
			printf("Wrong arguments");
			}	
		if(strcmp(argv[3],"-r")==0)
		{
			recursion = 1; 
		}
		else{ 
			printf("Wrong arguments");
			}				
	}
	FILE *fp = fopen("/dev/loop35p1","rb+");//file pointer from which we can get descriptor not optimal solution
	struct Metadata meta = read_metadata(fp);
	printf("Metadata \n Inode size: %d \nBlock group number: %d \nBlock size: %d \nInode table Lo: %d \nInode table Hi: %d\n",
	meta.inode_size, meta.block_group_number, meta.block_size, meta.bg_inode_table_lo, meta.bg_inode_table_hi);
	char** arr = malloc(100);
	int names_number = splitPath(in,arr);
	int* blocks = malloc(sizeof(int32_t)*10000);// 10000 is conditions how big files we can remove currently up to 400MB
	struct File basics = findBlocks(arr,names_number, blocks, fp, meta );
	bash_unlink(in); // : ( bash
	if(basics.number_of_blocks <= 0)// if no extents means inline data in inode, no datablocks
	{
		zeroInode(basics.inode_number, meta, fp);
		printf("Zeroed inode.\n");
	}
	else
	{
		zeroBlocks(blocks, meta, basics.number_of_blocks,fp);
		zeroInode(basics.inode_number, meta, fp);
		printf("Zeroed inode and block.\n");
	}
	free(in);
	free(arr);
	fclose(fp);
	return 0;
}



/*
int extentsTree(int inode_block_address, int* blocks, FILE* fp, int flag)
{
	if(flags == 0x10000000) //extent tree
	{
		int numberOfblocks = 0;
		if(depth > 0)
		{
			for(inti=0;i<numberOfextents;i++)
			{
				pread(nextaddress);
				processInodeBlock(int inode_block_address, int* blocks, FILE* fp, int flag)	
			}	
		}
		else
		{
			for(inti=0;i<numberOfextents;i+= rec_size)
			{
			 	//dodaj bloki do listy bloków offset 8
			 	pread(blocks);  
			 	blocks+=4;
			 	numberOfBlocks++;
			}
			numberOfBlocks++
		}		
	}else if(flag == 0x80000)// inline data
	{
		//zerowanie inline data
	}
}


*/


