#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <X11/Xlib.h>
#include <pthread.h>
#include <netdb.h>
#include <gtk/gtk.h>
#include <ctype.h>
#include "jsmn.h"
#define MAX_TOKENS 256


//gcc client_gtk.c -pthread -o client `pkg-config --cflags --libs gtk+-3.0` -export-dynamic -lX11

sem_t sala;		//semaforo binario
int cs;		//client socket
pthread_t sendt,recvt,ciclo,rec;	//threads
GtkBuilder *builder;
GtkWidget *view_port;
GtkWidget *home;
GtkWidget *sala_chat;
GtkWidget *fixed_home;
GtkWidget *entrar_sala;
GtkWidget *chat_bugs;
GtkWidget *entry_escolha_sala;
GtkWidget *label_bemvindo;
GtkWidget *label_recebido;
GtkWidget *label_erro_connect;
GtkWidget *salas_online_static;
GtkWidget *label_salas_on;
GtkWidget *fixed_salas;
GtkWidget *text_view;
GtkWidget *enviar_msg;
GtkWidget *entry_msg;
GtkWidget *label_numero_sala;
GtkWidget *scroll2;
GtkWidget *users_on;
GtkWidget *label_users_on;

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
void *ciclo_infinito();




void on_entry_msg_changed(GtkWidget *e) {
	printf("MUDEI ");
}

void on_enviar_msg_clicked(GtkWidget *b) {
	int len;
	char msg[500];
	sprintf(msg,"%s",gtk_entry_get_text(GTK_ENTRY(entry_msg)));
	char *json;
	json = criarJson(0,0,"broadcast",msg);
	len = write(cs,json,strlen(json));
		if(len < 0) {
			perror("message not sent");
			exit(1);
		}
	memset(msg,'\0',sizeof(msg));
}


void *recc(void *sock) {
int sv_sock = *((int *)sock);
	char msg[500];
	int len;
	while((len = recv(sv_sock,msg,500,0)) > 0) {
		msg[len] = '\0';
        PROTOCOL* protocol=parse_message(msg);
		if(atoi(protocol->source_id) == -1) {
			//mensagens do servidor
			if(strcmp(protocol->message_type,"choice_error")==0) {
				//recebido erro na escolha da sala
				gtk_label_set_text(GTK_LABEL(label_erro_connect),(const gchar*) protocol->content);
			} else if(strcmp(protocol->message_type,"show_rooms")==0){
				//mostrar as salas disponiveis
				printf("%s",protocol->content);
				gtk_label_set_text(GTK_LABEL(label_salas_on),(const gchar*) protocol->content);
			} else if(strcmp(protocol->message_type,"connected")==0) {
				//entrou com sucesso numa sala
				pthread_create(&ciclo,NULL,ciclo_infinito,NULL);
				pthread_exit(NULL);
			}
			memset(msg,'\0',sizeof(msg));
		}
	}
    return NULL;
} 











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
			if(strcmp(protocol->message_type,"choice_error")==0) {
				//recebido erro na escolha da sala
				gtk_label_set_text(GTK_LABEL(label_erro_connect),(const gchar*) protocol->content);
			} else if(strcmp(protocol->message_type,"show_rooms")==0){
				//mostrar as salas disponiveis
				printf("%s",protocol->content);
				gtk_label_set_text(GTK_LABEL(label_salas_on),(const gchar*) protocol->content);
			} else if(strcmp(protocol->message_type,"connected")==0) {
				//entrou com sucesso numa sala
				//sem_post(&sala);
			}
			memset(msg,'\0',sizeof(msg));
			continue;
		}
			//mensagens de utilizadores
			printf("User#%s: %s",protocol->source_id,protocol->content);
        	memset(msg,'\0',sizeof(msg));
	}
    return NULL;
}

void *sendmg(void *sock) {

		int len;
		char msg[500];
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
		len = write(cs,json,strlen(json));
		if(len < 0) {
			perror("message not sent");
			exit(1);
		}
		memset(msg,'\0',sizeof(msg));
	}
	close(cs);
	exit(1);			//acaba programa
}





void on_entrar_sala_clicked(GtkButton *b) {
	//botao entrar na sala
	char msg[100];
	sprintf(msg,"%s",gtk_entry_get_text(GTK_ENTRY(entry_escolha_sala)));
	int len;
	char *js = criarJson(0,-1,"choice",msg);
	len = write(cs,js,strlen(js));
		if(len < 0) {
			perror("Mensagem nao enviada");
			exit(1);
		}
		//ciclo_infinito();
}


GtkWidget* create_window (void){

   GError* error = NULL;
   GtkWidget *window;
   GtkBuilder *mainBuilder;

   mainBuilder = gtk_builder_new ();
   if (!gtk_builder_add_from_file (mainBuilder, "teste.glade", &error)){
      g_warning ("Couldn't load builder file: %s", error->message);
      g_error_free (error);
   }

   /* This is important */
   gtk_builder_connect_signals (mainBuilder, NULL);
   window = GTK_WIDGET (gtk_builder_get_object (mainBuilder, "sala_chat"));

   return window;
}


void  *ciclo_infinito() {
		// depois de entrar numa sala muda o contexto da interface
		printf("1 ... \n");
		//sem_wait(&sala);		//esoera que receba a mensagem do tipo "connected" do servidor
		printf("2 ... \n");
		sala_chat = create_window();
		printf("3 ... \n");
		gtk_widget_show(sala_chat);
		printf("4 ... \n");
		//gtk_widget_destroy(home);
		g_signal_connect(sala_chat,"destroy", G_CALLBACK(gtk_main_quit),NULL);
		
		printf("5 ... \n");
		gtk_main();
}


void on_criar_sala_clicked(GtkButton *b) {
	char msg[3];
	sprintf(msg,"%s","-1");
	int len;
	char *js = criarJson(0,-1,"choice",msg);
	len = write(cs,js,strlen(js));
		if(len < 0) {
			perror("Mensagem nao enviada");
			exit(1);
		}
		//ciclo_infinito();
}


void on_entry_escolha_sala_changed(GtkEntry *e) {

}

int main(int argc, char *argv[])
{
	XInitThreads();
	//GUI
	gtk_init(&argc,&argv);
	builder = gtk_builder_new_from_file("teste.glade");
	home = GTK_WIDGET(gtk_builder_get_object(builder,"home"));
	sala_chat = GTK_WIDGET(gtk_builder_get_object(builder,"sala_chat"));
	g_signal_connect(home,"destroy", G_CALLBACK(gtk_main_quit),NULL);
	gtk_builder_connect_signals(builder,NULL);
	fixed_home = GTK_WIDGET(gtk_builder_get_object(builder,"fixed_home"));
	entrar_sala = GTK_WIDGET(gtk_builder_get_object(builder,"entrar_sala"));
	chat_bugs = GTK_WIDGET(gtk_builder_get_object(builder,"chat_bugs"));
	entry_escolha_sala = GTK_WIDGET(gtk_builder_get_object(builder,"entry_escolha_sala"));
	label_bemvindo = GTK_WIDGET(gtk_builder_get_object(builder,"label_bemvindo"));
	label_recebido = GTK_WIDGET(gtk_builder_get_object(builder,"label_recebido"));
	label_erro_connect = GTK_WIDGET(gtk_builder_get_object(builder,"label_erro_connect"));
	salas_online_static= GTK_WIDGET(gtk_builder_get_object(builder,"salas_online_static"));
	label_salas_on= GTK_WIDGET(gtk_builder_get_object(builder,"label_salas_on"));
	fixed_salas = GTK_WIDGET(gtk_builder_get_object(builder,"fixed_salas"));
	text_view = GTK_WIDGET(gtk_builder_get_object(builder,"text_view"));
	enviar_msg = GTK_WIDGET(gtk_builder_get_object(builder,"enviar_msg"));
	entry_msg = GTK_WIDGET(gtk_builder_get_object(builder,"entry_msg"));
	view_port = GTK_WIDGET(gtk_builder_get_object(builder,"view_port"));
	label_numero_sala= GTK_WIDGET(gtk_builder_get_object(builder,"label_numero_sala"));
	scroll2 = GTK_WIDGET(gtk_builder_get_object(builder,"scroll2"));
	users_on= GTK_WIDGET(gtk_builder_get_object(builder,"users_on"));;
	label_users_on= GTK_WIDGET(gtk_builder_get_object(builder,"label_users_on"));
	gtk_widget_show(home);
	
	//sv side
	sem_init(&sala,0,0);			//semaforo binario
	struct sockaddr_in sv_addr;
	int client_sock;
	int sv_sock;
	int sv_addr_size;
	client_sock = socket(AF_INET,SOCK_STREAM,0);
	memset(sv_addr.sin_zero,'\0',sizeof(sv_addr.sin_zero));
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_port = htons(9001);
	sv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(connect(client_sock,(struct sockaddr *)&sv_addr,sizeof(sv_addr)) < 0) {
		gtk_label_set_text(GTK_LABEL(label_erro_connect),(const gchar*) "Erro de conexao");
		perror("erro conexao");
		exit(1);
	}
	cs = client_sock;
	printf("chat&bugs client terminal side \n");
	//pthread_create(&recvt,NULL,recvmg,&cs);
	pthread_create(&rec,NULL,recc,&cs);
	
	gtk_main();
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


