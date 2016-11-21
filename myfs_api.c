#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct alias_table{
	char fs_name[24];
	char alias_name[24];
} alias_table;

typedef struct disk_t{
	FILE *fs;
	char all_fsnames[1024][24];
	int currid;
	alias_table fs_alias[1024*2];
	int curralp;
} disk_t;

typedef struct meta_t{
	int fs_size;
	int block_size;	
} meta_t;

typedef struct inode_t{
	char type;
	short allocated_blocks;
	int size;
	int block_pointer;
} inode_t;

typedef struct dir_t{
	char **filenames;
	int *fileinodes;
	int num_of_files;
} dir_t;


int create_fs(char filename[], int sizeblock, int sizefs);
int load_fs(char fsname[]);
int init_fs_vars(int sizefs,int sizeblock);
inode_t* create_inode(char type, int size);
int create_file(char filename[], char *content);
dir_t* create_dir();
int get_datablock(int blocks_needed);
int get_inode();
int read_blocks(void *dest_ptr, int starting_block_id, int number_of_blocks);
int write_blocks(void *dest_ptr, int starting_block_id, int number_of_blocks);
int read_nbytes(void *dest_ptr, int starting_block_id, int nbytes);
int write_nbytes(void *dest_ptr, int starting_block_id, int nbytes);
int display_file(char filename[]);
int delete_file(char filename[]);
inode_t* read_inode(int inode_id);
int write_inode(inode_t *ptr, int inode_id);
int read_file(char filename[], char *dest_ptr);
int create_fs_name_alias(char old_fs_name[], char new_fs_name[]);
void printFileSystemList();
void printFileList();
void printAliasTable();
void printInodeBytemap();
void printDataBytemap();

const int root_inode = 2;
const int inode_size = sizeof(inode_t);

int inode_bytemap_block_id, data_bytemap_block_id;
int fs_size, block_size;
int ndata_blocks, blocks_for_dir;
int inode_start_block_id, data_start_block_id;
int max_files_per_dir = 256;
int loadedid = -1;

char inode_bytemap[1024*16];
char data_bytemap[1024*16];

disk_t disk;
meta_t meta;


int start_sfs(){
	disk.currid=0;
	disk.curralp=0;
	return 0;
}

int init_fs_vars(int sizefs,int sizeblock){
	int bitmap_block_size;
	int nblocks_for_inode;
	int i;
	
	meta.fs_size = sizefs;
	meta.block_size = sizeblock;
	
	inode_bytemap_block_id = ceil((float)(sizeof(meta_t)) / meta.block_size);
	
	bitmap_block_size = ceil((float)(meta.fs_size / ((meta.block_size + inode_size + 2) * meta.block_size)));
	bitmap_block_size = bitmap_block_size > 0 ? bitmap_block_size : 1;
	data_bytemap_block_id = inode_bytemap_block_id + bitmap_block_size;
	
	inode_start_block_id = data_bytemap_block_id + bitmap_block_size;
	
	ndata_blocks = floor((float)(meta.fs_size - ((inode_start_block_id + 1) * meta.block_size)) / (meta.block_size + inode_size));
	nblocks_for_inode = ceil((float)(ndata_blocks * inode_size) / meta.block_size);
	nblocks_for_inode = nblocks_for_inode > 0 ? nblocks_for_inode : 1;
	data_start_block_id = inode_start_block_id + nblocks_for_inode;
	
	blocks_for_dir = ceil((float)16 * max_files_per_dir / meta.block_size);
	
	for (i = 0; i < ndata_blocks; ++i){
		data_bytemap[i] = '0';
	}
	for (i = 0; i < ndata_blocks; ++i){
		inode_bytemap[i] = '0';
	}
	inode_bytemap[2] = '1';
	
	return 0;
}


int get_datablock(int blocks_needed){
	int i = 0,j = 0;
	
	while(i < ndata_blocks){
		if(data_bytemap[i] == '0'){
			for(j = i;j < blocks_needed+i;j++){
				if(data_bytemap[j] == '1')
					break;
			}
			if(j == blocks_needed+i){
				for(j = i;j < blocks_needed+i;j++){
					data_bytemap[j] = '1';
				}
				return i;
			}

		}
		i++;
	}
	return -1;
}

int get_inode(){
	for (int i = 0; i < ndata_blocks; ++i){
		if(inode_bytemap[i] == '0'){
			inode_bytemap[i] = '1';
			return i;
		}
	}
	return -1;
}

int read_blocks(void *dest_ptr, int starting_block_id, int number_of_blocks){
	fseek(disk.fs, meta.block_size*starting_block_id, SEEK_SET);
	fread(dest_ptr, 1, meta.block_size*number_of_blocks, disk.fs);
    return 0;
}

int read_nbytes(void *dest_ptr, int starting_block_id, int nbytes){
	fseek(disk.fs, meta.block_size*starting_block_id, SEEK_SET);
	fread(dest_ptr, nbytes, 1, disk.fs);
    return 0;
}

int write_blocks(void *dest_ptr, int starting_block_id, int number_of_blocks){
	fseek(disk.fs, meta.block_size*starting_block_id, SEEK_SET);
	fwrite(dest_ptr, 1, meta.block_size*number_of_blocks, disk.fs);
	return 0;
}

int write_nbytes(void *dest_ptr, int starting_block_id, int nbytes){
	fseek(disk.fs, meta.block_size*starting_block_id, SEEK_SET);
	fwrite(dest_ptr, 1, nbytes, disk.fs);
	return 0;
}

inode_t* read_inode(int inode_id){
	char *buf;
	buf = (char*)malloc(sizeof(char)*inode_size);
	fseek(disk.fs, inode_start_block_id*meta.block_size+(inode_id*inode_size), SEEK_SET);
	fread(buf, 1, inode_size, disk.fs);
	return (inode_t *)buf;
}

int write_inode(inode_t *ptr, int inode_id){
	fseek(disk.fs, inode_start_block_id*meta.block_size + (inode_id*inode_size), SEEK_SET);
	fwrite((char *)ptr, 1, inode_size, disk.fs);
	return 0;
}

int read_file(char filename[], char *dest_ptr){
	inode_t *root, *ptr;
	root = read_inode(root_inode);
	char *rootbuff;
	rootbuff = (char *)malloc(blocks_for_dir * meta.block_size);
	read_blocks(rootbuff, data_start_block_id + root->block_pointer, blocks_for_dir);
	dir_t *rootdir = (dir_t*)rootbuff;
	for (int i = 0; i < rootdir->num_of_files; ++i){
		if(!strcmp(rootdir->filenames[i], filename)){
			ptr = read_inode(rootdir->fileinodes[i]);
			read_nbytes(dest_ptr, data_start_block_id + ptr->block_pointer, ptr->size);
			dest_ptr[ptr->size] = '\0';
			return 0;
		}
	}
	printf("Error! reading file: \"%s\" file does not exist in specified file system\n",filename);
	return 1;
}

int display_file(char filename[]){
	inode_t *root, *ptr;
	char dest_ptr[1024*128];
	char *rootbuff;
	root = read_inode(root_inode);
	rootbuff = (char *)malloc(blocks_for_dir * meta.block_size);
	read_blocks(rootbuff, data_start_block_id + root->block_pointer, blocks_for_dir);
	dir_t *rootdir = (dir_t*)rootbuff;
	for (int i = 0; i < rootdir->num_of_files; ++i){
		if(!strcmp(rootdir->filenames[i], filename)){
			ptr = read_inode(rootdir->fileinodes[i]);
			read_nbytes(dest_ptr, data_start_block_id + ptr->block_pointer, ptr->size);
			printf("%s\n",dest_ptr);
			free(rootbuff);
			return 0;
		}
	}
	printf("Error reading file: \"%s\" file does not exist in specified file system\n",filename);
	return 1;
}

inode_t* create_inode(char type, int size){
	inode_t *new = (inode_t*)malloc(sizeof(inode_t));
	new->size = size;
	new->type = type;
	new->allocated_blocks = blocks_for_dir;
	return new;
}

int create_file(char filename[], char *content){
	inode_t *iroot;
	int j;
	char *rootdata;
	
	iroot = read_inode(root_inode);
	rootdata = (char *)malloc(blocks_for_dir * meta.block_size);
	read_blocks(rootdata, data_start_block_id + iroot->block_pointer, blocks_for_dir);
	dir_t* rootdir = (dir_t*)rootdata;
	
	if(rootdir->num_of_files > max_files_per_dir){
		printf("Maximum file limit reached. Directory can not contain more files\n");
		return 1;
	}
		
	for(j = 0; j<rootdir->num_of_files; j++){
		if(!strcmp(rootdir->filenames[j], filename)){
			printf("\"%s\\%s\" file already exists!! \n", disk.all_fsnames[loadedid], rootdir->filenames[j] );
			return 1;
		}
	}
	
	read_blocks(inode_bytemap, inode_bytemap_block_id, (data_bytemap_block_id - inode_bytemap_block_id));
	read_blocks(data_bytemap, data_bytemap_block_id, (data_bytemap_block_id - inode_bytemap_block_id));
	
	int id = get_inode();
	if(id == -1){
		printf("Cannot write more files! Out of inodes\n");
		return 1;
	}
	
	inode_t *ptr = create_inode('f', strlen(content));
	ptr->allocated_blocks = ceil((float)(strlen(content) + 1) / meta.block_size);
	ptr->block_pointer = get_datablock(ptr->allocated_blocks);
	
	if(ptr->block_pointer == -1){
		printf("Cannot write more files! Out of datablock\n");
		return 1;
	}
	
	ptr->size = strlen(content) + 1;	
	write_blocks(content, data_start_block_id + ptr->block_pointer, ptr->allocated_blocks);
	write_inode(ptr, id);
	
	rootdir->filenames[rootdir->num_of_files] = (char *) malloc(strlen(filename) + 1);
	strcpy(rootdir->filenames[rootdir->num_of_files], filename);
	rootdir->fileinodes[rootdir->num_of_files] = id;
	rootdir->num_of_files++;
	write_blocks((char*)rootdir, data_start_block_id + iroot->block_pointer, blocks_for_dir);
	free(rootdata);
	
	iroot->size += ptr->size;
	write_inode(iroot, root_inode);
	free(iroot);
	
	write_blocks(inode_bytemap, inode_bytemap_block_id, (data_bytemap_block_id - inode_bytemap_block_id));
	write_blocks(data_bytemap, data_bytemap_block_id, (data_bytemap_block_id - inode_bytemap_block_id));
	
	return 0;
}

int delete_file(char filename[]){
	inode_t *iroot, *ptr;
	int i, j;
	char *rootdata;
	
	iroot = read_inode(root_inode);
	rootdata = (char *)malloc(blocks_for_dir * meta.block_size);
	read_blocks(rootdata, data_start_block_id + iroot->block_pointer, blocks_for_dir);
	dir_t* rootdir = (dir_t*)rootdata;
		
	for(i = 0; i < rootdir->num_of_files; i++){
		if(!strcmp(rootdir->filenames[i], filename)){
			break;
		}
	}
	
	if(i == rootdir->num_of_files){
		printf("Error! File not found\n");	
		return 1;
	}
	
	read_blocks(inode_bytemap, inode_bytemap_block_id, (data_bytemap_block_id - inode_bytemap_block_id));
	read_blocks(data_bytemap, data_bytemap_block_id, (data_bytemap_block_id - inode_bytemap_block_id));
	
	inode_bytemap[rootdir->fileinodes[i]] = 0;
	ptr = read_inode(rootdir->fileinodes[i]);
	for(j = 0; j < ptr->allocated_blocks ; j++){
		data_bytemap[ptr->block_pointer+j] = 0;
	}
	
	for(j = i; j < rootdir->num_of_files-1; j++){
		free(rootdir->filenames[j]);
		rootdir->filenames[j] = (char *) malloc(strlen(rootdir->filenames[j + 1]) + 1);	
		strcpy(rootdir->filenames[j], rootdir->filenames[j + 1]);
		rootdir->fileinodes[j] = rootdir->fileinodes[j + 1];
	}
	
	rootdir->num_of_files--;
	write_blocks((char*)rootdir, data_start_block_id + iroot->block_pointer, blocks_for_dir);
	free(rootdata);
	
	iroot->size -= ptr->size;
	write_inode(iroot, root_inode);
	free(iroot);
	
	write_blocks(inode_bytemap, inode_bytemap_block_id, (data_bytemap_block_id - inode_bytemap_block_id));
	write_blocks(data_bytemap, data_bytemap_block_id, (data_bytemap_block_id - inode_bytemap_block_id));
	
	return 0;
}

dir_t* create_dir(){
	dir_t *new = (dir_t*)malloc(sizeof(dir_t));
	new->filenames=(char **)malloc(sizeof(char *)*max_files_per_dir);
	new->fileinodes=(int *)malloc(sizeof(int *)*max_files_per_dir);
	new->num_of_files =0;
	return new;
}


int create_fs(char filename[], int sizeblock, int sizefs){
	for (int i = 0; i < disk.currid; ++i){
		if(!strcmp(filename, disk.all_fsnames[i])){
			printf("The file \"%s\" is already a disk.fs!!\n", filename);
			return 1;
		}
	}
	
	for (int i = 0; i < disk.curralp; ++i){
		if(!strcmp(filename, disk.fs_alias[i].alias_name)){
			printf("The file \"%s\" is already used as a fs alias!!\n", filename);
			return 1;
		}
	}
	
	init_fs_vars(sizefs, sizeblock);
	
	strcpy(disk.all_fsnames[disk.currid], filename);
	if(loadedid != -1){
	fclose(disk.fs);
	}
	disk.fs = fopen(filename, "w+");
	loadedid = disk.currid;

	write_blocks((char *)(&meta), 0, inode_bytemap_block_id);
		
	dir_t *rootdir = create_dir();
	inode_t *root = create_inode('d', sizeof(rootdir));
	root->block_pointer = get_datablock(blocks_for_dir);
	write_blocks((char*)rootdir, data_start_block_id + root->block_pointer, blocks_for_dir);
	
	write_inode(root, root_inode);
	
	write_blocks(inode_bytemap, inode_bytemap_block_id, (data_bytemap_block_id - inode_bytemap_block_id));
	write_blocks(data_bytemap, data_bytemap_block_id, (data_bytemap_block_id - inode_bytemap_block_id));
	
	disk.currid++;
	
	return 0;
}

int load_fs(char fsname[]){
	meta_t tmpmeta;
	char backup[1024*1024*4];
	int flag = 0;
	int i, j;
	for (i = 0; i < disk.currid && !flag; ++i){
		if(!strcmp(fsname, disk.all_fsnames[i])){
			if(loadedid == i){
				return 0;
			}
			loadedid = i;
			flag = 1;
		}
	}
	for (i = 0; i < disk.curralp && !flag; ++i){
		if(!strcmp(fsname, disk.fs_alias[i].alias_name)){
			for(j=0; j<disk.currid; j++){
				if(!strcmp(disk.fs_alias[i].fs_name, disk.all_fsnames[j])){
					if(loadedid == j){
						return 0;
					}
					loadedid = j;
					break;
				}
			}
			flag = 1;
		}
	}
	
	if(flag){
		fclose(disk.fs);
			
		disk.fs = fopen(disk.all_fsnames[loadedid],"r+");
		
		fseek(disk.fs, 0, SEEK_SET);
		fread((char *)&tmpmeta, sizeof(meta_t), 1, disk.fs);
			
		read_blocks(backup, 0, ndata_blocks + data_start_block_id);
		fclose(disk.fs);
		disk.fs = fopen(disk.all_fsnames[loadedid], "w+");
		fwrite(backup, 1, tmpmeta.fs_size, disk.fs);
		init_fs_vars(tmpmeta.fs_size, tmpmeta.block_size);
			
		return 0;
	}

	printf("Error! File System \"%s\" not found. Create one before use\n", fsname);
	return 1;
}

int create_fs_name_alias(char old_fs_name[], char new_fs_name[]){
	int i, j;
	
	for(i=0; i < disk.currid; i++){
		if(strcmp(disk.all_fsnames[i], old_fs_name) == 0){
			break;
		}
	}
	
	for(j=0; j < disk.curralp; j++){
		if(strcmp(disk.fs_alias[j].alias_name, old_fs_name) == 0){
			strcpy(disk.fs_alias[j].alias_name, new_fs_name);
			return 0;
		}
	}
	
	if(i == disk.currid){
		printf("Error! No such eixsting fs \"%s\"\n",old_fs_name);
		return 1;
	}
	
	for(i=0; i < disk.curralp; i++){
		if(strcmp(disk.fs_alias[i].alias_name, new_fs_name) == 0){
			strcpy(disk.fs_alias[i].fs_name, old_fs_name);
			return 0;
		}
		if(strcmp(disk.fs_alias[i].fs_name, old_fs_name) == 0){
			strcpy(disk.fs_alias[i].alias_name, new_fs_name);
			return 0;
		}
	}
	
	strcpy(disk.fs_alias[disk.curralp].fs_name, old_fs_name);
	strcpy(disk.fs_alias[disk.curralp].alias_name, new_fs_name); 
	
	disk.curralp++;
	return 0;	
}

void printFileSystemList(){
	int i, j;
	if(disk.currid == 0){
		printf("No file system found. Create one before use\n");
		return;
	}
	printf("*** List of Available File Systems ***\n");
	for(i = 0; i < disk.currid; i++){
		for(j = 0; j < disk.curralp; j++){
			if(!strcmp(disk.all_fsnames[i], disk.fs_alias[j].fs_name)){
				printf("%s (%s)\n", disk.fs_alias[j].fs_name, disk.fs_alias[j].alias_name);
				break;
			}
		}
		if(j == disk.curralp)
			printf("%s\n",disk.all_fsnames[i]);
	}
}

void printFileList(){
	inode_t *i, *ptr ;
	char *data;
	int j;
	
	i = read_inode(root_inode);
	
	data = (char *)malloc(blocks_for_dir*meta.block_size);
	read_blocks(data, data_start_block_id + i->block_pointer, blocks_for_dir);
	dir_t* rootdir = (dir_t*)data;
	if(rootdir->num_of_files == 0){
		printf("*** No file in fs \"%s\" ***\n",disk.all_fsnames[loadedid]);
		return;
	}
	printf("*** %d file in fs \"%s\" ***\n",rootdir->num_of_files, disk.all_fsnames[loadedid]);
	for(j = 0; j < rootdir->num_of_files; j++){
		ptr = read_inode(rootdir->fileinodes[j]);
		printf("%s\t%d byte\n", rootdir->filenames[j], ptr->size );
		free(ptr);
	}
	free(data);
}

void printAliasTable(){
	int i;
	printf("*** Printing the fs table ***\n");
	for(i = 0; i < disk.currid; i++){
		printf("%s\n",disk.all_fsnames[i]);
	}
	printf("*** Printing the alias table ***\n");
	for(i = 0; i < disk.curralp; i++){
		printf("%s \t %s\n",disk.fs_alias[i].fs_name, disk.fs_alias[i].alias_name);
	}

}

void printInodeBytemap(){
	int x = 0;
	char *imap;
	
	imap = (char *)malloc(sizeof(char)*(data_bytemap_block_id - inode_bytemap_block_id)*meta.block_size);
	read_blocks(imap, inode_bytemap_block_id, (data_bytemap_block_id - inode_bytemap_block_id));
	for(x = 0; x < ndata_blocks; x++){
		printf("%c ", imap[x] );
	}
	printf("\n");
}

void printDataBytemap(){
	char *dmap;
	int x = 0;
	
	dmap = (char *)malloc(sizeof(char)*(data_bytemap_block_id - inode_bytemap_block_id) * meta.block_size);
	read_blocks(dmap, data_bytemap_block_id, (data_bytemap_block_id - inode_bytemap_block_id));
	for(x = 0; x < ndata_blocks; x++){
		printf("%c ", dmap[x] );
	}
	printf("\n");
}
