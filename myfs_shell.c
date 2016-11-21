#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include "myfs_api.c"

void start(){
	write(1,"myfs> ",6);	
}

void execute(char **cmd, int n){
	int fssize, blocksize;
	int tmp;
	char tmpbuf[24];
	char content[1024];
	int i,j;
	char cpbuf[1024*1024];
		
		 	
  	if (!strcmp(cmd[0], "mkfs")){
		if(n != 4){
			write(2,"usage: mkfs <fsname> <block_size> <fs_size>[MB/KB/ ]\n",56);
			return;
		}else{
			blocksize = atoi(cmd[2]);
			tmp = strlen(cmd[3]);
			if (cmd[3][tmp-2]=='M')	fssize = 1024*1024;
			else if (cmd[3][tmp-2]=='K')	fssize = 1024;
			else fssize = atoi(cmd[2]);
			cmd[3][tmp-1] = cmd[3][tmp-2] = '\0';
			create_fs(cmd[1], blocksize, fssize * atoi(cmd[3])); 
		}
	}
	
	else if(!strcmp(cmd[0], "use")){
		if(n != 4){
			write(2,"usage: use <old_fs_name> as <new_file_name>\n",45);
			return;
		}else{
			load_fs(cmd[1]);
			create_fs_name_alias(cmd[1],cmd[3]);
		}
	}
	
	else if(!strcmp(cmd[0], "ls")){
		if(n > 2){
			write(2,"usage: ls [<fs_name>]\n",23);
			return;
		}else if(n == 1){
			printFileSystemList();
		}else{
			if(!load_fs(cmd[1])){
				printFileList();
			}
		}
	}
	
	else if(!strcmp(cmd[0], "write")){
		content[0]='\0';
		if (n < 4 && (!strcmp(cmd[2],"<<"))){
			write(2,"usage: write <filepath> << <containt>\n", 38);
			return;
		}
		for(i = 0; cmd[1][i] != '/' && cmd[1][i] != '\0'; i++){
			tmpbuf[i] = cmd[1][i];
		}
		tmpbuf[i]='\0';
		i++;
		if(!load_fs(tmpbuf)){
			for(j = 0; i != strlen(cmd[1]); j++,i++){
				tmpbuf[j] = cmd[1][i];				
			}
			tmpbuf[j] = '\0';
			for(i = 3; i < n; i++){
				if(i == 3){
					sprintf(content,"%s", cmd[i]);	
				}else{
					sprintf(content,"%s %s", content, cmd[i]);
				}
			}
			if(create_file(tmpbuf, content)){
				char res[8];
				printf("Want to Append (a) or Overwrite (w) or Nothing (n) ? : ");			
				scanf("%s",res);
				if(res[0] == 'a'){
					read_file(tmpbuf, cpbuf);
					delete_file(tmpbuf);
					sprintf(cpbuf, "%s %s", cpbuf, content);
					create_file(tmpbuf, cpbuf);
				}else if(res[0] == 'w'){
					delete_file(tmpbuf);
					create_file(tmpbuf, content);			
				}else{
					return;
				}
			}
		}
	}
	
	else if(!strcmp(cmd[0], "read")){
		if (n != 2){
			write(2,"usage: read <filepath>\n", 24);
			return;
		}
		for(i = 0; cmd[1][i] != '/' && cmd[1][i] != '\0'; i++){
			tmpbuf[i] = cmd[1][i];
		}
		tmpbuf[i]='\0';
		i++;
		if(!load_fs(tmpbuf)){
			for(j = 0; i != strlen(cmd[1]); j++,i++){
				tmpbuf[j] = cmd[1][i];				
			}
			tmpbuf[j] = '\0';
			display_file(tmpbuf);
		}	
	}
	
	else if(!strcmp(cmd[0], "cp")){
		if (n != 3){
			write(2,"usage: cp <src_file> <dest_file>\n", 34);
			return;
		}
		
		for(i = 0; cmd[1][i] != '/' && cmd[1][i] != '\0'; i++){
			tmpbuf[i] = cmd[1][i];
		}
		tmpbuf[i]='\0';
		if(cmd[1][i] == '\0'){
			FILE *fp = fopen(cmd[1], "r");
			fseek(fp, 0, SEEK_END);
			long fsize = ftell(fp);
			rewind(fp);
			fread(cpbuf, fsize, 1, fp);
			fclose(fp);
		}else{
			i++;
			if(!load_fs(tmpbuf)){
				for(j = 0; i != strlen(cmd[1]); j++,i++){
					tmpbuf[j] = cmd[1][i];				
				}
				tmpbuf[j] = '\0';
				if(read_file(tmpbuf, cpbuf)){
					return;
				}
			}else{
				return;
			}
		}
		
		for(i = 0; cmd[2][i] != '/' && cmd[2][i] != '\0'; i++){
			tmpbuf[i] = cmd[2][i];
		}
		tmpbuf[i]='\0';
		if(cmd[2][i] == '\0'){
			FILE *fp = fopen(cmd[2], "w");
			fwrite(cpbuf, strlen(cpbuf), 1, fp);
			fclose(fp);
		}else{
			i++;
			if(!load_fs(tmpbuf)){
				for(j = 0; i != strlen(cmd[2]); j++,i++){
					tmpbuf[j] = cmd[2][i];				
				}
				tmpbuf[j] = '\0';
				if(create_file(tmpbuf, cpbuf)){
					delete_file(tmpbuf);
					create_file(tmpbuf, cpbuf);
				}
			}
		}	
	}
	
	else if(!strcmp(cmd[0], "rm")){
		if(n!=2){
			write(2,"usage: rm <file>\n", 18);
			return;
		}
		for(i = 0; cmd[1][i] != '/' && cmd[1][i] != '\0'; i++){
			tmpbuf[i] = cmd[1][i];
		}
		tmpbuf[i]='\0';
		i++;
		if(!load_fs(tmpbuf)){
			for(j = 0; i != strlen(cmd[1]); j++,i++){
				tmpbuf[j] = cmd[1][i];				
			}
			tmpbuf[j] = '\0';
			delete_file(tmpbuf);
		}
	}
	
	else if(!strcmp(cmd[0], "mv")){
		if (n != 3){
			write(2,"usage: mv <src_file> <dest_file>\n", 34);
			return;
		}
		for(i = 0; cmd[1][i] != '/' && cmd[1][i] != '\0'; i++){
			tmpbuf[i] = cmd[1][i];
		}
		tmpbuf[i]='\0';
		i++;
		if(!load_fs(tmpbuf)){
			for(j = 0; i != strlen(cmd[1]); j++,i++){
				tmpbuf[j] = cmd[1][i];				
			}
			tmpbuf[j] = '\0';
			if(read_file(tmpbuf, cpbuf) || delete_file(tmpbuf)){
				return;
			}
		}else{
			return;
		}
		
		for(i = 0; cmd[2][i] != '/' && cmd[2][i] != '\0'; i++){
			tmpbuf[i] = cmd[2][i];
		}
		tmpbuf[i]='\0';
		i++;
		if(!load_fs(tmpbuf)){
			for(j = 0; i != strlen(cmd[2]); j++,i++){
				tmpbuf[j] = cmd[2][i];				
			}
			tmpbuf[j] = '\0';
			if(create_file(tmpbuf, cpbuf)){
				return;
			}
		}else{
			return;
		}
	}

	
	
}


char **commandparser(char *line,int *len){
	char **parsedcmd;
	char *t;
	char *i,*marker;
	char *tmp;
	int cnt = 0;
	int termcnt, termsize, cmdcnt, j, k;
	
	*len = 0;
	
	if(*line == '\n'){
		return(char **)0;
	}
	
	for(t = line; *t != '\n' && *t != '\0';){
		
			if(*t != ' ' && *t != '\t' && *t != '\n' && *t != '\0'){	
				(*len)++;
				t++;
				
				while(*t != ' ' && *t != '\t' && *t != '\n' && *t != '\0')	t++;
				
				while((*t == ' ' || *t == '\t') && *t != '\n' && *t != '\0')	t++;
			}
	}
	
	parsedcmd=(char **)malloc(((*len)+1)*sizeof(char **));
	parsedcmd[(*len)]=(char *)0;
	termcnt=0;
	cmdcnt=0;
	marker=line;	
	t=line;
	
	while(1){
		while(*t == ' ' || *t == '\t'){
			t++;
		}
		if(*t == '\n' || *t == '\0'){
			break;	
		}else{
			for(tmp = t; *tmp != ' ' && *tmp != '\n' && *tmp != '\0'; tmp++);
			termsize = (tmp - t);
			parsedcmd[termcnt] = (char *)malloc((termsize + 1) * sizeof(char));
			for(k=0; k < termsize && *t != '\n' && *t != '\0'; k++){
				parsedcmd[termcnt][k] = *t;
				t++;
			}
			parsedcmd[termcnt][k] = '\0';
			termcnt++;
		}
	}
	return parsedcmd;
}


void shell(){
	char  line[1024];
	char **command=(char **)0;
	char *ch;
	int redirection,pipe;
	int commandlen;
    start();
    line[read(0,line,1024)]='\0';
    command=commandparser(line,&commandlen);
   	
	if(command!=(char **)0){
		if(command!=(char **)0 && strcmp(command[0],"exit")==0)
			_exit(0);
		else	
			execute(command,commandlen);
	}
	shell();	
}


int main(){
	start_sfs();
	shell();
	return 0;
}
