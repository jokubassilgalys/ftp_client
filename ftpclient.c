#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <zlib.h>

//#define HOST "ftp.gnu.org"
//#define HOST "speedtest.tele2.net"
#define HOST "ftp.dlptest.com"
#define PORT "21"
#define BUFFER_SIZE 2048

int command_socket;
int transfer_socket;
int fcount;

void print_error(char* msg){
	fprintf(stderr, "%s\n", msg);
   exit(EXIT_FAILURE);
}

/*void zip_add_file(zipFile zf, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Error opening file %s: %s\n", filename, strerror(errno));
        return;
    }

    zip_fileinfo zi;
    memset(&zi, 0, sizeof(zip_fileinfo));

    if (zipOpenNewFileInZip(zf, filename, &zi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK) {
        printf("Error opening new file in zip: %s\n", filename);
        fclose(file);
        return;
    }

    char buffer[16384];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, CHUNK, file)) > 0) {
        if (zipWriteInFileInZip(zf, buffer, bytes_read) != ZIP_OK) {
            printf("Error writing file in zip: %s\n", filename);
            zipCloseFileInZip(zf);
            fclose(file);
            return;
        }
    }

    if (ferror(file)) {
        printf("Error reading file %s: %s\n", filename, strerror(errno));
    }

    if (zipCloseFileInZip(zf) != ZIP_OK) {
        printf("Error closing file in zip: %s\n", filename);
    }

    fclose(file);
}*/


void flush_stdin(){
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
}

/*int connect_other(char host[], char port[]){
	
	struct addrinfo hints, *result, *res;
	int socket_fd;
	//char buffer[BUFFER_SIZE];
	
	memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   
   char h[14], p[6];
   printf("host:");
   scanf("%s", &h);
   printf("port:");   
   scanf("%s", &p);
  	//printf("%s;\n", host);

	if (getaddrinfo(h, p, &hints, &result) != 0) {
		print_error("getaddrinfo error");
   }

   for (res = result; res != NULL; res = res->ai_next) {
		socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (socket_fd == -1) continue;
      if (connect(socket_fd, res->ai_addr, res->ai_addrlen) == 0) break;
	}
	freeaddrinfo(result);
	
	if (res == NULL) {
		print_error("failed to connect");
	}

	printf("OTHER SOC CONNECTED\n");
	flush_stdin();
	
	return socket_fd;
}*/

void get_ip_and_port(char buffer[]){
	char ip[14], port[6], tmp[4];
	char* c;
	int start, end, p1, p2;

	memset(tmp, 0, sizeof tmp);
	memset(ip, 0, sizeof ip);
	
	c = strchr(buffer, (int)'(');
	start = (int)(c - buffer);

	c = strchr(buffer, (int)',');
	end = (int)(c - buffer);
	
	strncpy(tmp, buffer+start+1, end-start-1);
	strcpy(ip, tmp);
	//printf("start:%d; end:%d; tmp:%s; ip:%s; L%d\n", start, end, tmp, ip, strlen(ip));	
	
	for(int i = 0; i < 3; i++){
		strcat(ip, ".");
		
		start = end;
		c = strchr(buffer+end+1, (int)',');
		end = (int)(c - buffer);
		
		memset(tmp, 0, sizeof tmp);
		strncpy(tmp, buffer+start+1, end-start-1);
		strcat(ip, tmp);
		//printf("start:%d; end:%d; tmp:%s; ip:%s; L%d\n", start, end, tmp, ip, strlen(ip));	
	}
	strcat(ip, "\0");
	//printf("start:%d; end:%d; tmp:%s; ip:%s; L%d\n", start, end, tmp, ip, strlen(ip));	
	//ip[14] = '\0';
	
	start = end;
	c = strchr(buffer+end+1, (int)',');
	end = (int)(c - buffer);

	memset(tmp, 0, sizeof tmp);
	strncpy(tmp, buffer+start+1, end-start-1);
	//printf("start:%d; end:%d; tmp:%s; L%d\n", start, end, tmp, strlen(tmp));
	strcat(tmp, "\0");
	p1 = atoi(tmp);
	
	start = end;
	c = strchr(buffer+end+1, (int)')');
	end = (int)(c - buffer);

	memset(tmp, 0, sizeof tmp);
	strncpy(tmp, buffer+start+1, end-start-1);
	//printf("start:%d; end:%d; tmp:%s; L%d\n", start, end, tmp, strlen(tmp));
	p2 = atoi(tmp);
		
	
	printf("ip:%s\n", ip);
	printf("p1:%d, p2:%d\n", p1, p2);
	int portnum = (p1*256)+p2;
	sprintf(port, "%d", portnum);
	strcat(port, "\0");
	printf("port:%s\n", port);


	struct addrinfo hints, *result, *res;
	int socket_fd;
	//char buffer[BUFFER_SIZE];
	
	memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(ip, port, &hints, &result) != 0) {
		print_error("getaddrinfo error");
   }

   for (res = result; res != NULL; res = res->ai_next) {
		socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (socket_fd == -1) continue;
      if (connect(socket_fd, res->ai_addr, res->ai_addrlen) == 0) break;
	}
	freeaddrinfo(result);
	
	if (res == NULL) {
		printf("failed to establish connection with server");
		return;
	}

	printf("SOCKET CONNECTED\n");
	flush_stdin();
	
	transfer_socket = socket_fd;
}

int receive(int socket_fd, char buffer[], int buff_size){
	int r;
	char response_code[4];
	memset(buffer, 0, buff_size);
	
	if((r = recv(socket_fd, buffer, buff_size, 0)) > 0){
		printf("received:%s\n", buffer);
		
		strncpy(response_code, buffer, 4);
		if(response_code[3] != ' ' && response_code[3] != '-') return -2;
		
		if(response_code[3] == '-'){
			for(int i = r-2; i > 0; i--){
				if(buffer[i] == '\n'){
					if(buffer[i+4] == '-'){
						receive(socket_fd, buffer, buff_size);
						break;
					}
					else if(buffer[i+4] == ' ' && isdigit(buffer[i+2])){
						response_code[3] = '\0';
						return atoi(response_code);
					}
				}
			}
		}
		else{
			response_code[3] = '\0';
			if(atoi(response_code) == 227) get_ip_and_port(buffer);
			return atoi(response_code);
		}
	}
	else {
		return -1;
	}
}

int send_command(int socket_fd, char send_buffer[]){
	int r;
	strcat(send_buffer, "\n");
	if((r = send(socket_fd, send_buffer, strlen(send_buffer), 0)) > 0){
		printf("sent:%s\n", send_buffer);
	}
	else {
		printf("failed to send");
	}
	return r;
}

void list_files(char path[], char extension[], char *files[]){
	DIR *d;
	struct dirent *dir;
	if(strlen(path) == 0) path = ".";
	d = opendir(path);
		
	if(d){
		while((dir = readdir(d)) != NULL) {
			char full_path[96];
			snprintf(full_path, sizeof(full_path), "%s/%s", path, dir->d_name);
			
			if(dir->d_type == DT_DIR){
				if(strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0){
					continue;
				}
				list_files(full_path, extension, files);
			}
			else{
				char* c = strchr(dir->d_name, (int)'.');
				if(c == NULL) continue;
				int dot = (int)(c - (dir->d_name));
				char tmp[10];
				strcpy(tmp, dir->d_name+dot);
				
				//printf("ex:%s, tmp:%s\n", extension, tmp);
				if(strcmp(extension, tmp) == 0){
					printf("t:%s", full_path);
					files[fcount] = malloc(strlen(full_path)+1);
					strcpy(files[fcount], full_path);
					fcount++;
				}
				else{
					printf("f:%s\n", dir->d_name);
				}
			}
		}
		closedir(d);
	}
}

void send_file(int socket_fd, char filename[]){
	char send_buffer[BUFFER_SIZE];
	FILE* file;
	file = fopen(filename, "r");

	printf("filename:%s;\n", filename);	
	memset(send_buffer, 0, sizeof send_buffer);
	
	if(file == NULL){
		printf("cannot open file");
		return;
	}
	while(fgets(send_buffer, BUFFER_SIZE, file)){
		printf("sending:%s\n", send_buffer);
		send(socket_fd, send_buffer, strlen(send_buffer), 0);
	}
	close(socket_fd);
	fclose(file);
}

void receive_file(int socket_fd, char filename[]){
	int r;
	char buffer[BUFFER_SIZE];
	
	memset(buffer, 0, sizeof buffer);
	r = receive(transfer_socket, buffer, sizeof buffer);
	printf("receiving file: %s\n", filename);
	
	FILE* file;
	file = fopen(filename, "w");
	fprintf(file, buffer);
	fclose(file);
}

void handle_mget(char arg[], int mode){
	
	char* c;
	int start, end, file_count = 0;
	char extension[10], tmp[10], filename[32], buffer[BUFFER_SIZE*2];
	char *files[32] = {0};
	
	strcpy(extension, arg+1);
		
	memset(buffer, 0, sizeof buffer);
	strcpy(buffer, "PASV");
	send_command(command_socket, buffer); //PASV
	receive(command_socket, buffer, sizeof buffer); //227
	
	memset(buffer, 0, sizeof buffer);
	strcpy(buffer, "LIST");
	send_command(command_socket, buffer); //LIST
	receive(command_socket, buffer, sizeof buffer); //150
	recv(transfer_socket, buffer, BUFFER_SIZE*2, 0);
	//receive(transfer_socket, buffer, sizeof buffer);
	printf("buffer:%s;", buffer); //get list
	//receive(command_socket, buffer, sizeof buffer); //226
	//flush_stdin();
	//printf("buffer:%s;", buffer);
	
	char line[128];
	c = strchr(buffer, (int)'\n');
	//end = (int)(c - buffer);
	start = 0;
	
	while(c != NULL){
		end = (int)(c - buffer);
		printf("start:%d, end:%d\n", start, end);
		
		memset(line, 0, sizeof line);
		strncpy(line, buffer+start, end-start-1);
		printf("LINE:%s;\n", line);
		
		c = strrchr(line, (int)'.');
		if(c != NULL){
			int dot = (int)(c - line);
			strcpy(tmp, line+dot);
			if(strcmp(tmp, extension) == 0){
				printf("tmp:%s; ex:%s;\nLINE:%s\n", tmp, extension, line);
				c = strrchr(line, (int)' ');
				if(c != NULL){
					int name_end = (int)(c - line);
					strcpy(filename, line+name_end+1);
					files[file_count] = malloc(strlen(filename)+1);
					printf("FILENAME:%s\n", filename);
					strcpy(files[file_count], filename);
					file_count++;
				}
			}
		}
		start = end + 1;
		c = strchr(buffer + start, (int)'\n');
	}
	
	receive(command_socket, buffer, sizeof buffer); //226
	
	for(int i = 0; i < file_count; i++){
		printf("FILE:%s\n", files[i]);
		memset(buffer, 0, sizeof buffer);
		strcpy(buffer, "PASV");
		send_command(command_socket, buffer); //PASV
		receive(command_socket, buffer, sizeof buffer); //227
		
		memset(buffer, 0, sizeof buffer);
		strcpy(buffer, "RETR ");
		strcat(buffer, files[i]);
		strcat(buffer, "\n");
		//printf("buffer:%s;\n", buffer);
		send_command(command_socket, buffer); //RETR file
		receive(command_socket, buffer, sizeof buffer); //150
		
		receive_file(transfer_socket, files[i]); //get file
		receive(command_socket, buffer, sizeof buffer); //226
		printf("done\n");
	}
	
	if(mode == 1){
		char sys_buffer[BUFFER_SIZE];
		memset(sys_buffer, 0, sizeof sys_buffer);
		strcpy(sys_buffer, "zip files");
		
		for(int i = 0; i < file_count; i++){
			strcat(sys_buffer, " ");
			strcat(sys_buffer, files[i]);	
		}
		printf("sys:%s;", sys_buffer);
		system(sys_buffer);
		
		memset(sys_buffer, 0, sizeof sys_buffer);
		strcpy(sys_buffer, "rm");
		for(int i = 0; i < file_count; i++){
			strcat(sys_buffer, " ");
			strcat(sys_buffer, files[i]);
		}
		printf("sys:%s;", sys_buffer);
		system(sys_buffer);
	}
}

void handle_mput(char arg[], int mode){
	fcount = 0;
	char* c;
	int star;
	char path[64], extension[10], fname[32], buffer[BUFFER_SIZE];
	char *files[32] = {0};
	
	c = strchr(arg, (int)'*');
	if(c == NULL) return;
	star = (int)(c - arg);
	
	memset(path, 0, sizeof(path));
	strncpy(path, arg, star-1);
	
	memset(extension, 0, sizeof(extension));
	strcpy(extension, arg+star+1);

	printf("arg:%s;\npath:%s;\nextension:%s\n", arg, path, extension);
	
	list_files(path, extension, files);
	
	if(mode == 0){
		for(int i = 0; i < fcount; i++){
		strcat(files[i], "\0");
		printf("FILE:%s, %d\n", files[i], strlen(files[i]));
		
		c = strrchr(files[i], (int)'/');
		star = (int)(c - files[i]);
		
		strcpy(fname, files[i]+star+1);		
		memset(buffer, 0, sizeof buffer);
		strcpy(buffer, "PASV");
		send_command(command_socket, buffer);
		receive(command_socket, buffer, sizeof buffer);
		
		memset(buffer, 0, sizeof buffer);
		strcpy(buffer, "STOR ");
		strcat(buffer, fname);
		strcat(buffer, "\n");
		send_command(command_socket, buffer);
		receive(command_socket, buffer, sizeof buffer);
		send_file(transfer_socket, files[i]);
		receive(command_socket, buffer, sizeof buffer);
		}
	}
	else if(mode == 1){
		char sys_buffer[BUFFER_SIZE];
		memset(sys_buffer, 0, sizeof sys_buffer);
		strcpy(sys_buffer, "zip files");
		
		for(int i = 0; i < fcount; i++){
			strcat(sys_buffer, " ");
			strcat(sys_buffer, files[i]);	
		}
		printf("sys:%s;", sys_buffer);
		system(sys_buffer);
		
		memset(buffer, 0, sizeof buffer);
		strcpy(buffer, "PASV");
		send_command(command_socket, buffer);
		receive(command_socket, buffer, sizeof buffer);
		flush_stdin();

		memset(buffer, 0, sizeof buffer);
		strcpy(buffer, "STOR files.zip\n");
		send_command(command_socket, buffer);
		receive(command_socket, buffer, sizeof buffer);
		
		char fbuff[BUFFER_SIZE];
		memset(fbuff, 0, sizeof fbuff);
		strcpy(fbuff, "files.zip");		
		
		send_file(transfer_socket, fbuff);
		receive(command_socket, buffer, sizeof buffer);
	}
	printf("done.");
	//receive(command_socket, buffer, sizeof buffer);
}

int main(int argc, char *argv[])
{
	struct addrinfo hints, *result, *res;
	char buffer[BUFFER_SIZE];
	
	memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   if (getaddrinfo(HOST, PORT, &hints, &result) != 0) {
		print_error("getaddrinfo error");
   }
   
   for (res = result; res != NULL; res = res->ai_next) {
		command_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (command_socket == -1) continue;
      if (connect(command_socket, res->ai_addr, res->ai_addrlen) == 0) break;
	}
	freeaddrinfo(result);
	
	if (res == NULL) {
		print_error("failed to connect ftp server");
	}

	int r, state = -1;
	char send_buffer[BUFFER_SIZE];
	char filename[64];
		
	for(;;){
		r = receive(command_socket, buffer, sizeof buffer);
		printf("ftp>");
		//TODO get ip and port here
		
		//226 transfer complete
		if(r == 150){
			//file is ok, prepare for transfer
			if(state == 0){
				//receive file
				receive_file(transfer_socket, filename);
			}
			else if(state == 1){
				//send file
				send_file(transfer_socket, filename);
			}
			else if(state == 2){
				//recieve list
				memset(buffer, 0, sizeof buffer);
				r = receive(transfer_socket, buffer, sizeof buffer);
			}
			state = -1;
		}
		else if(r == 221){
			//connection ends
			close(transfer_socket);
			close(command_socket);
			print_error("connection ended");
		}
		else if(r == -1){
			//failed to receive return code
			printf("receive failed\n");
		}
		else{
			//continue 
			memset(send_buffer, 0, sizeof send_buffer);
			scanf("%[^\n]s", send_buffer);
			
			char tmp[6];
			strncpy(tmp, send_buffer, 5);
			if(strcmp(tmp, "retr ") == 0 || strcmp(tmp, "RETR ") == 0){
				state = 0;
				strcpy(filename, send_buffer+5);
			}
			else if(strcmp(tmp, "stor ") == 0 || strcmp(tmp, "STOR ") == 0){
				state = 1;
				strcpy(filename, send_buffer+5);
			}
			else if(strcmp(tmp, "list") == 0 || strcmp(tmp, "LIST") == 0){
				state = 2;
			}
			else if(strcmp(tmp, "mput ") == 0 || strcmp(tmp, "MPUT ") == 0){
				state = -2;
				strcpy(filename, send_buffer+5);
				handle_mput(filename, 0);
			}
			else if(strcmp(tmp, "mget ") == 0 || strcmp(tmp, "MGET") == 0){
				state = -2;
				strcpy(filename, send_buffer+5);
				printf("FILEex:%s;", filename);
				handle_mget(filename, 0);
			}
			else if(strcmp(tmp, "mzput") == 0 || strcmp(tmp, "MZPUT") == 0){
				state = -2;
				strcpy(filename, send_buffer+6);
				handle_mput(filename, 1);
			}
			else if(strcmp(tmp, "mzget") == 0 || strcmp(tmp, "MZGET") == 0){
				state = -2;
				strcpy(filename, send_buffer+6);
				
				printf("FILEex:%s;", filename);
				handle_mget(filename, 1);
			}
			
			int len = (sizeof send_buffer) / (sizeof send_buffer[0]);
			if(len > 1 /*&& state >= -1*/){
				printf("reached\n");
				if(r = send_command(command_socket, send_buffer) == 0){
					print_error("sent nothing\n");
				}
			}
		}
		flush_stdin();
	}    
       
	return 0;
}
