#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>
#include "jsmn.h"

#define MAX_TOKENS 256


typedef struct protocol {
	char* source_id;
	char* destiny_id;
	char* message_type;
	char* content;
	char* timestamp;
} PROTOCOL;

char* get_token(jsmntok_t* t, char* message);
PROTOCOL* parse_message(char* message);
void print_protocol(PROTOCOL* protocol);
char *criarJson(int source_id,int destiny_id,char* message_type,char* content);




void *recvmg(void *sock)
{
	int sv_sock = *((int *)sock);
	char msg[500];
	int len;
	while((len = recv(sv_sock,msg,500,0)) > 0) {
		msg[len] = '\0';
        PROTOCOL* protocol=parse_message(msg);
		if(atoi(protocol->source_id) == -1) {
			//mensagens do servidor
			printf("%s",protocol->content);
			memset(msg,'\0',sizeof(msg));
			continue;
		}
			//mensagens de utilizadores
			printf("User#%s: %s",protocol->source_id,protocol->content);
        	memset(msg,'\0',sizeof(msg));
	}
    return NULL;
}
int main(int argc, char *argv[])
{
	struct sockaddr_in sv_addr;
	int client_sock;
	int sv_sock;
	int sv_addr_size;
	pthread_t st,recvt;
	char msg[500];
	int len;


	









	client_sock = socket(AF_INET,SOCK_STREAM,0);
	memset(sv_addr.sin_zero,'\0',sizeof(sv_addr.sin_zero));
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_port = htons(9001);
	sv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(connect(client_sock,(struct sockaddr *)&sv_addr,sizeof(sv_addr)) < 0) {
		perror("connection not esatablished");
		exit(1);
	}

	printf("BEM-VINDO AO CHAT&BUGS, ESCOLHE UMA SALA PARA ENTRAR ( OU CRIA UMA ) \n");

	// thread para receber as mensagens
	pthread_create(&recvt,NULL,recvmg,&client_sock);

	int escolha;
	scanf("%d",&escolha);
	char auxiliar[10];
	
	sprintf(auxiliar,"%d",escolha);

	char *js = criarJson(0,-1,"choice",auxiliar);
	len = write(client_sock,js,strlen(js));
		if(len < 0) {
			perror("message not sent");
			exit(1);
		}
	//memset(js,'\0',sizeof(js));





	while(fgets(msg,500,stdin) > 0) {
		char *json;
		if(msg[0] == 'p' && msg[2] == '_' ) {
			// mensagem privada
			json= criarJson(0,atoi(&msg[1]),"private",msg);
			printf("Enviou em privado para %d a msg: %s \n",atoi(&msg[1]),msg);

        }else if(msg[0] == 'b' && msg[1] == 'a' && msg[2] == 'n' && msg[3] == '_'){
            //comando para banir um utilizador da sala caso este seja moderador
            msg[5] = '\0';
            //char aux = msg[4];
            json = criarJson(0,atoi(&msg[4]),"ban","");
            
        }else if (msg[0] == 'o' && msg[1] == 'n' && msg[2] == '_') {
			// comando para ver os utilizadores online na sala
			json = criarJson(0,0,"whoisonline",msg);

		} else {
			//mensagem para toda a gente
			json = criarJson(0,0,"broadcast",msg);
		}
		len = write(client_sock,json,strlen(json));
		if(len < 0) {
			perror("message not sent");
			exit(1);
		}
		memset(msg,'\0',sizeof(msg));
	}
	pthread_join(recvt,NULL);
	close(client_sock);

}




char *criarJson(int source_id,int destiny_id,char* message_type,char* content) {
    char *msg=malloc(600);
    sprintf(msg,"{\"source_id\":\"%d\",\"destiny_id\":\"%d\",\"message_type\":\"%s\",\"content\":\"%s\", \"timestamp\": \"2019-04-10 00:12:12\"}",source_id,destiny_id,message_type,content);
    return msg;
}


char* get_token(jsmntok_t* t, char* message)
{
	int token_size = (t->end - t->start);
	char* token = (char*)malloc(sizeof(char) * (token_size + 1)); //allocate memory to the token
	strncpy(token,message+t->start, token_size); //copy token
	token[token_size]='\0'; //close the token
	return token;
}

PROTOCOL* parse_message(char* message){
	PROTOCOL* protocol=(PROTOCOL*)malloc(sizeof(PROTOCOL));
	jsmntok_t tokens[MAX_TOKENS];
	jsmn_parser parser;
	jsmn_init(&parser);
	jsmn_parse(&parser, message, strlen(message), tokens, MAX_TOKENS);
	
	for(int i=0; i<256; i++)
	{
		jsmntok_t* t = &tokens[i];
		if(t->end <= 0) break;
		char* token = get_token(t, message);
		
		if(strcmp("source_id", token) == 0)
		{
			free(token);
			i++;
			jsmntok_t* t = &tokens[i];
			char* token=get_token(t, message);
			protocol->source_id = (char*) malloc(sizeof(char) * (strlen(token) + 1));
			strcpy(protocol->source_id, token);
		}
		else if(strcmp("destiny_id", token) == 0)
		{
			free(token);
			i++;
			jsmntok_t* t = &tokens[i];
			char* token = get_token(t, message);
			protocol->destiny_id = (char*) malloc(sizeof(char) * (strlen(token) + 1));
			strcpy(protocol->destiny_id, token);
		}
		else if(strcmp("message_type", token)==0)
		{
			free(token);
			i++;
			jsmntok_t* t = &tokens[i];
			char* token = get_token(t,message);
			protocol->message_type = (char*) malloc(sizeof(char) * (strlen(token) + 1));
			strcpy(protocol->message_type, token);
		}
		else if(strcmp("content", token)==0)
		{
			free(token);
			i++;
			jsmntok_t* t = &tokens[i];
			char* token = get_token(t, message);
			protocol->content = (char*) malloc(sizeof(char) * (strlen(token) + 1));
			strcpy(protocol->content, token);
		}
		else if(strcmp("timestamp", token)==0)
		{
			free(token);
			i++;
			jsmntok_t* t = &tokens[i];
			char* token = get_token(t, message);
			protocol->timestamp = (char*) malloc(sizeof(char) * (strlen(token) + 1));
			strcpy(protocol->timestamp, token);
		}
	}
	
	return protocol;
}

void print_protocol(PROTOCOL* protocol)
{
	printf("source_id: %s\ndestiny_id: %s\nmessage_type: %s\ncontent: %s\ntimestamp: %s\n",protocol->destiny_id, protocol->source_id, protocol->message_type, protocol->content, protocol->timestamp);
}

