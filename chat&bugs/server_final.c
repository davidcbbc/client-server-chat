#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>
#include <errno.h>
#include "jsmn.h"
#define MAX_TOKENS 256




typedef struct protocol {
    char* source_id;
    char* destiny_id;
    char* message_type;
    char* content;
    char* timestamp;
} PROTOCOL;

typedef struct sala_info {
    int mod;                                    //socket do primeiro client ou ID ( moderador )
    pthread_t id;                               // id do thread
    int sala_id;                                //id da sala
    int n_utilizadores;                         //numero de utilizadores na sala
    fd_set allset;
    int clients[5];                             //array de sockets da sala
    struct sala_info *next;                     //prox sala ( lista ligada )
    struct lista_clientes *utilizadores;        //lista ligada com os utilizadores
}SALA;


typedef struct lista_salas {
    // Estrutura que contém a lista ligada das salas
    SALA *pfirst;
}LISTA_SALAS;

struct c_info {
    // Estrutra que contém as informações do cliente no servidor
    SALA *sala_atual;    //sala atual do cliente
    int id;        // detem o id do user
    int c_socket;    // detem o socket associado
    struct c_info *next;    // detem o proximo utilizador ( lista ligada)
};

typedef struct lista_clientes {
    // Estrutura que contém a lista ligada dos clientes
    struct c_info *pfirst;
}LISTAC;







char* get_token(jsmntok_t* t, char* message);
PROTOCOL* parse_message(char* message);
void print_protocol(PROTOCOL* protocol);
char *criarJson(int source_id,int destiny_id,char* message_type,char* content);
int procuraID(LISTAC lista , int id);
void addClient(LISTAC *lista ,int socket,int id, SALA *sala);
void rmClient(LISTAC *lista,int id,SALA *sala_a_sair);
int procuraMax(LISTAC list);
void enviarutilizadores(int sock , int c_id,SALA salita);
void criarSala(int mod);
int procura_id_salas_max(void);
int registar_user_sala(int sock , int numero_sala, struct c_info *temporary);
void* client_handler(void *attr);
SALA *procura_id_salas(int id_sala);
void sala_handler_novo_user(int sig);
struct c_info procura_ultimo_user_sala(SALA salita);
void ban(int id_moderador ,int id_banido ,SALA *salita);
void enviarusers(int socket, SALA salita);





int n = 0;                       // NUMERO TOTAL DE CLIENTES LIGADOS AO SERVIDOR
LISTA_SALAS salas={NULL};        //LISTA LIGADA COM AS SALAS



// SE O SOURCE_ID FOR -1 SIGNIFICA QUE SAO MENSAGENS DO SERVIDOR

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t recec;    // semaforo rececionista
sem_t consu;    // semaforo consumidor
pthread_t thread_sala[20];




void enviartodos(char *msg,int client_id,SALA sala)
{
    struct c_info *temp = sala.utilizadores->pfirst;
    char *msg2 = criarJson(client_id,0,"broadcast",msg);
    for( int i = 0 ; i < sala.n_utilizadores ; i ++) {
        //printf("SOCKET DA POSICAO %d --> %d \n",i,temp->c_socket);
        if(send(temp->c_socket,msg2,strlen(msg2),0) < 0) {
            perror("Falha ao enviar enviar todos");
        }
        temp = temp->next;
    }
}

void enviarprivado(char *msg, int client_id,PROTOCOL * pro,SALA sala) {
    int socket;
    sprintf(msg,"*PRIVADO* %s ",pro->content);
    LISTAC *lista = sala.utilizadores;
    
    
    
    if((socket = procuraID(*lista,atoi(pro->destiny_id)))) {
        char *msg2 = criarJson(client_id,atoi(pro->destiny_id),"private",msg);
        if(send(socket,msg2,strlen(msg2),0) < 0) {     //
            perror("Falha ao enviar procura ID");
        }
        return;
    }
    
    // caso nao haja ninguem com esse id
    socket = procuraID(*lista,client_id);
    sprintf(msg,"Nao existe nenhum utilizador com esse ID ! \n");
    char *msg2 = criarJson(-1,client_id,"private",msg);
    if(send(socket,msg2,strlen(msg2),0) < 0) {     //
        perror("Falha ao enviar procura ID 2");
    }
    
}


void enviarutilizadores( int sock , int c_id,SALA salita) {
    // envia os utilizadores presentes na sala de chat atual
    char msg[100];
    char aux[50];
    struct c_info *temp;
    LISTAC *lista = salita.utilizadores;
    
    temp = lista->pfirst;
    //printf("RECEBI UM ENVIAR UTILIZADORES \n");
    sprintf(msg,"Utilizadores presentes na sala: \n");
    
    
    for( int i = 0 ; i < salita.n_utilizadores ; i++) {
        sprintf(aux,"User#%d \n",temp->id);
        strcat(msg,aux);
        temp = temp->next;
    }
    
    
    char *msg2 = criarJson(-1,c_id,"whoisonline",msg);
    if(send(sock,msg2,strlen(msg2),0) < 0) {     //
        perror("Falha ao enviar criar Json");
    }
}

void enviarsalas(int socket) {
    // envia as salas de chat presentes no servidor
    
    char msg[100];
    char aux[50];
    
    //sprintf(msg,"Salas disponiveis no CHAT&BUGS : \n");
    
    SALA *temp = salas.pfirst;
    while(1) {
        if(temp == NULL) {
            sprintf(msg,"SEM SALAS CRIADAS \n");
            break;
        }
        if(temp != NULL) {
            sprintf(aux,"Sala#%d \n",temp->sala_id);
            strcat(msg,aux);
        }
        if(temp->next == NULL) break;
        temp = temp->next;
    }
    
    char *msg2 = criarJson(-1,0,"show_rooms",msg);
    if(send(socket,msg2,strlen(msg2),0) < 0) {     //
        perror("Falha ao enviar criarJson");
    }
    
}



void sala_handler_novo_user(int sig) {
    //printf("recebi um sinal NO HANDLER\n");
}


void ban(int id_moderador ,int id_banido ,SALA *salita) {
    // esta funcao da ban a um utilizador de uma determinada salita
    printf("MODERADOR DA SALA %d UTILIZADOR QUE PEDIU BAN %d \n",salita->mod,id_moderador);
    if (id_moderador == salita->mod) {
        // quer dizer que é mesmo o moderador
        struct c_info *banido = salita->utilizadores->pfirst;
        while(banido->id != id_banido) {
            //procura o cliente banido na lista de clientes da sala
            if(banido->next == NULL) {
                // utilizador nao existe
                char *msg_non_user=malloc(200);
                sprintf(msg_non_user,"Esse utilizador não está on-line \n");
                char *msg2 = criarJson(-1,id_moderador,"priv_non_usr",msg_non_user);
                struct c_info *mod = salita->utilizadores->pfirst;
                while(mod->c_socket != id_moderador) mod = mod->next;
                
                if(send(mod->c_socket,msg2,strlen(msg2),0) < 0) {     //
                    perror("Falha ao enviar criar Json");
                }
                return;
            }
            banido = banido->next;
        }
        
        printf("utilizador %d banido na sala %d \n",banido->id,salita->sala_id);
        FD_CLR(banido->c_socket,&salita->allset);
        char *msg_desconectado=malloc(200);
        sprintf(msg_desconectado,"User#%d banido do servidor pelo moderador!\n",banido->id);
        enviartodos(msg_desconectado,-1,*salita);
        rmClient(salita->utilizadores,banido->id,salita);
        pthread_mutex_lock(&mutex);
        //decremente o numero total de users online
        n--;
        pthread_mutex_unlock(&mutex);
        return;
        } else {
        // envia mensagem a dizer que so os moderadores podem banir utilizadores
            printf("NAO E MODERADOR\n");
        char *msg_non_moderator=malloc(200);
        sprintf(msg_non_moderator,"Apenas o moderador pode banir pessoas \n");
        char *msg2 = criarJson(-1,id_moderador,"priv_non_mod",msg_non_moderator);
        
        struct c_info *mod = salita->utilizadores->pfirst;
        while(mod->c_socket != id_moderador) mod = mod->next;
        
        if(send(mod->c_socket,msg2,strlen(msg2),0) < 0) {     //
            perror("Falha ao enviar criar Json");
        }
        
    }
}


void enviarusers(int socket, SALA salita) {
    // envia os users on na presente sala
    
    char msg[100];
    char aux[50];
    
    sprintf(msg,"Users on-line na Sala#%d : \n",salita.sala_id);
    
    struct c_info *temp = salita.utilizadores->pfirst;
    
    while(1) {
        if(temp == NULL) {
            sprintf(msg,"SEM USERS ON-LINE NA SALA \n");
            break;
        }
        if(temp != NULL) {
            sprintf(aux,"User#%d \n",temp->id);
            strcat(msg,aux);
        }
        if(temp->next == NULL) break;
        temp = temp->next;
    }
    
    char *msg2 = criarJson(-1,0,"show_users",msg);
    if(send(socket,msg2,strlen(msg2),0) < 0) {     //
        perror("Falha ao enviar criarJson");
    }
    
}


void *salas_thread(void *attr)
{
    // FUNCAO RESPONSAVEL PELO TRATAMENTO DO THREAD DA SALA
    sleep(1);                 // espera que o criarSala() fique com o ID do thread
    
    
    struct c_info *cl = malloc(sizeof(struct c_info));
    // procura a sala
    SALA * esta_sala = salas.pfirst;
    
    pthread_t temporario;
    temporario = esta_sala->id;
    //printf("temporario_id %d \n thread_id %d \n",(int)temporario,(int)pthread_self());
    while(!pthread_equal(temporario, pthread_self())) {
        printf("entrei no ciclo \n");
        if(esta_sala->next != NULL) esta_sala = esta_sala->next;
        temporario = esta_sala->id;
        sleep(1);
    }
    
    
    cl = esta_sala->utilizadores->pfirst;
    
    
    //printf("THREAD CRIADO COM O SOCKET %d , ID DE USER %d , NA SALA %d  \n",cl->c_socket,cl->id,esta_sala->sala_id);
    
    
    sigset_t set;
    sigset_t orig_mask;
    struct sigaction act;
    memset (&act, 0, sizeof(act));
    act.sa_handler = sala_handler_novo_user;
    sigaction(SIGUSR1, &act, 0);
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);   // adiciona o sinal SIGUSR1 ao set
    if ( sigprocmask(SIG_BLOCK, &set, &orig_mask) < 0)
    perror(" a ");
    
    
                             
    //sigprocmask(SIG_UNBLOCK,&set,NULL);
    //s = pthread_sigmask(SIG_UNBLOCK, &set, NULL);                // descbloqueia o sinal no set
    //if (s != 0)    perror("sigmask (salas_thread) ");
    int s;
    
    
    
    
    
    
    //Set ao primeiro
    FD_SET(cl->c_socket,&esta_sala->allset);

    //socket maximo para o pselect()
    int maxfd = cl->c_socket;

    
    int res;

    fd_set rset;

    while(1) {
        FD_ZERO(&rset);
        rset = esta_sala->allset;
        
        if( esta_sala->n_utilizadores == 0) {
            //printf("A ELEMINAR SALA \n");
            //elemina sala se nao houver ninguem
            SALA *sala_temp = salas.pfirst;
            SALA *sala_prev = NULL; //aux
            while(1) {
                //eleminar sala
                if(sala_temp->sala_id == esta_sala->sala_id) {
                    printf("A eleminar sala com o id #%d \n",esta_sala->sala_id);
                    if(sala_prev == NULL) {
                        //primeira sala a ser eleminada
                        salas.pfirst = NULL;
                        free(sala_temp);
                    } else {
                        sala_prev->next=sala_temp->next;
                        free(sala_temp);
                    }

                    break;
                }
                sala_prev = sala_temp;
                sala_temp= sala_temp->next;
            }
            pthread_exit(NULL);
            break;
        }

   
        //printf("NUMERO TOTAL DE USERS NA SALA %d :  %d \n",esta_sala->sala_id,esta_sala->n_utilizadores);
        
       


        res = pselect (maxfd + 1, &rset, NULL, NULL, NULL, &orig_mask);

        if(res < 0 && errno == EINTR) {
            //recebeu um sinal
            cl = esta_sala->utilizadores->pfirst;
            //printf("recebido sinal de novo user na sala#%d \n",esta_sala->sala_id);
            FD_ZERO(&esta_sala->allset);
            for( int k = 0 ; k < esta_sala->n_utilizadores ; k++) {
            //FD_SET A TODOS OS SOCKETS DOS CLIENTES
            if(esta_sala->utilizadores->pfirst == NULL) {
                break;
            }
            //printf("ciclo FD_SET: ID %d SOCKET %d \n",cl->id,cl->c_socket);
            FD_SET(cl->c_socket, &esta_sala->allset);
            if(cl ->next == NULL) {
                maxfd = cl->c_socket;
                //printf("NOVO SOCKET MAXIMO %d \n",maxfd);
                break;
                }
            cl = cl->next;
            }
            continue;
        }
        
        
        if(res == 0) {
            printf("recebi um zerito /n");
            continue;
        }

        char msg[500];
        int len;
        
        cl = esta_sala->utilizadores->pfirst;
        for( int p = 0 ; p < esta_sala->n_utilizadores ; p++) {
            if(!FD_ISSET(cl->c_socket,&rset)){
                //caso nao esteja set
                cl = cl->next;
            } else {
                //printf("ID QUE FALOU %d DO SOCKET %d \n",cl->id,cl->c_socket);
                
                if((len = recv(cl->c_socket,msg,500,0)) > 0) {
                    msg[len] = '\0';
                    PROTOCOL* protocol=parse_message(msg);
                    if(strcmp(protocol->message_type,"broadcast")==0) enviartodos(msg,cl->id,*esta_sala);
                    else if(strcmp(protocol->message_type,"private")==0) enviarprivado(msg,cl->id,protocol,*esta_sala);
                    else if(strcmp(protocol->message_type,"whoisonline")==0) enviarusers(cl->c_socket, *esta_sala);
                    else if(strcmp(protocol->message_type,"show_rooms")==0) enviarsalas(cl->c_socket);
                    else if(strcmp(protocol->message_type,"show_users")==0) enviarusers(cl->c_socket,*esta_sala);
                    else if(strcmp(protocol->message_type,"ban")==0) ban(cl->c_socket,atoi(protocol->destiny_id),esta_sala);
                    memset(msg,'\0',sizeof(msg));
                    break;
                }
                
                if(len == -1) {
                    //entrou um novo utilizador , temos que dar continue
                    printf("len = -1\n");
                    continue;
                }
                
                if(len == 0){
                    
                    // CASO O UTILIZADOR SAIA DA SALA
                    //printf("A REMOVER UTILIZADOR %d \n",cl->id);
                    FD_CLR(cl->c_socket,&esta_sala->allset);
                    char *msg_desconectado=malloc(200);
                    sprintf(msg_desconectado,"User#%d desconectado do servidor!\n",cl->id);
                    enviartodos(msg_desconectado,-1,*cl->sala_atual);
                    printf("user#%d desconectado da sala %d\n",cl->id,cl->sala_atual->sala_id);
                    memset(msg_desconectado,'\0',sizeof(&msg_desconectado));
                    if(cl->c_socket == esta_sala->mod) {
                        //caso seja o moderador
                        if(esta_sala->n_utilizadores > 1) {
                            //caso haja mais utilizadores para alem do moderador
                            esta_sala->mod = cl->next->c_socket;
                            printf("Novo mod da sala #%d : User#%d \n",esta_sala->sala_id,cl->next->id);
                            char *msg_novo_mod=malloc(200);
                            sprintf(msg_novo_mod,"User#%d é agora o novo moderador da sala\n",cl->next->id);
                            enviartodos(msg_novo_mod,-1,*cl->sala_atual);
                            memset(msg_novo_mod,'\0',sizeof(&msg_novo_mod));
                        }
                    }     
                    
                    rmClient(esta_sala->utilizadores,cl->id,cl->sala_atual);
                    pthread_mutex_lock(&mutex);
                    //decremente o numero total de users online
                    n--;
                    pthread_mutex_unlock(&mutex);
                }
            }
            
        }
    }
    return NULL;
}


void* client_handler(void *attr) {
    sem_wait(&consu);
    int cl_socket = *(int*)attr;
    printf("entrou novo utilizador de socket %d \n",cl_socket);
    enviarsalas(cl_socket);
    
    char msg[500];
    int len,numero_sala;
    memset(msg,'\0',sizeof(msg));
    
    if((len = recv(cl_socket,msg,500,0)) > 0) {
        msg[len] = '\0';
        PROTOCOL* protocol=parse_message(msg);
        numero_sala = atoi(protocol->content);
        memset(msg,'\0',sizeof(msg));

    } else perror("erro recv");
    memset(msg,'\0',sizeof(msg));
    int status;
    
    //printf("numero_sala = %d \n",numero_sala);
    if(numero_sala == -1) {
        //caso queira criar a sala
        criarSala(cl_socket);
        pthread_mutex_lock(&mutex);
        n++;
        pthread_mutex_unlock(&mutex);
        
        printf("user#1 criou a sala %d e agora é mod \n",procura_id_salas_max());
        char msg_connected[200];
         sprintf(msg,"connected");
            char *msg4 = criarJson(-1,1,"connected",msg);
            if(send(cl_socket,msg4,strlen(msg4),0) < 0) { 
                perror("Falha ao enviar status 1");
            }

        sprintf(msg_connected,"Bem-vindo , moderador ! \n");
        char *msg2 = criarJson(-1,1,"private",msg_connected);
        if(send(cl_socket,msg2,strlen(msg2),0) < 0) {     //
            perror("Falha ao enviar criar sala");
        }
        
        
    } else if(numero_sala > 0  && numero_sala < 20){
        //caso nao queira criar a sala
        struct c_info *temporary = malloc(sizeof(struct c_info));
        
        status = registar_user_sala(cl_socket , numero_sala , temporary);
        //printf("status %d \n",status);
        if(status == 1) {
            //caso consiga entrar na sala
            pthread_mutex_lock(&mutex);
            n++;
            pthread_mutex_unlock(&mutex);
            printf("user#%d conectado na sala %d \n",temporary->id,numero_sala);
            sprintf(msg,"connected");
            char *msg2 = criarJson(-1,temporary->id,"connected",msg);
            if(send(cl_socket,msg2,strlen(msg2),0) < 0) { 
                perror("Falha ao enviar status 1");
            }
            char msg_connected[200];
            sprintf(msg_connected,"User#%d conectou-se à sala ! \n",temporary->id);
            enviartodos(msg_connected,temporary->id,*procura_id_salas(numero_sala));
            pthread_kill(thread_sala[numero_sala-1],SIGUSR1);
        }else if(status == -1){
            // caso a sala esteja cheia
            sprintf(msg,"A sala esta de momento cheia ! \n");
            char *msg2 = criarJson(-1,temporary->id,"choice_error",msg);
            if(send(cl_socket,msg2,strlen(msg2),0) < 0) {     //
                perror("Falha ao enviar status -1");
            }
        } else {
            //printf("sala nao existe \n");
            sprintf(msg,"A sala que queres entrar nao existe ! \n");
            char *msg2 = criarJson(-1,temporary->id,"choice_error",msg);
            if(send(cl_socket,msg2,strlen(msg2),0) < 0) {     //
                perror("Falha ao enviar status else");
            }
        }
    }
    //envia sinal que ja podem utilizar o rececionista outra vez e o thread acaba
    sem_post(&recec);
    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
    printf("A inicializar CHAT&BUGS server\n");
    struct sockaddr_in sv_addr,client_addr;
    int sv_sock, client_sock;
    socklen_t client_addr_size;
    // THREADS PARA ENVIAR E RECEBER MENSAGENS
    pthread_t handler;
    sv_sock = socket(AF_INET,SOCK_STREAM,0);
    memset(sv_addr.sin_zero,'\0',sizeof(sv_addr.sin_zero));
    sv_addr.sin_family = AF_INET;
    sv_addr.sin_port = htons(9001);
    sv_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr_size = sizeof(client_addr);
    if(bind(sv_sock,(struct sockaddr *)&sv_addr,sizeof(sv_addr)) != 0) {
        perror("binding nao sucedido");
        exit(1);
    }
    
    if(listen(sv_sock,5) != 0) {
        perror("listening nao sucedido");
        exit(1);
    }
    
    //semaphore_create(mach_task_self(), &recec, SYNC_POLICY_FIFO, 3);
    // existem no maximo 3 rececionistas para usar a funcao handler
    
    sem_init(&recec,0,3);       //comeca com 3 conecoes de rececionista de cada vez
    sem_init(&consu,0,0);

    while(1) {
        
        
        sem_wait(&recec);
        if((client_sock = accept(sv_sock,(struct sockaddr *)&client_addr,&client_addr_size)) < 0) {
            perror("accept nao sucedido");
            exit(1);
        }
        pthread_create(&handler,NULL,client_handler,&client_sock);
        sem_post(&consu);
    }
}

int registar_user_sala(int sock , int numero_sala, struct c_info *temporary) {
    //caso consigo entrar na sala retorna 1 , caso nao haja a sala retorna 0 , caso esteja cheio retorna -1
    if(salas.pfirst == NULL) return 0;
    SALA *temp = salas.pfirst;
    while(1) {
        if(temp->sala_id == numero_sala) {
            // acertou no id da sala
            if(temp-> n_utilizadores == 5) return -1;        //caso esteja cheia
            //adiciona o cliente
            temp->clients[temp->n_utilizadores] = sock;
            temp->n_utilizadores++;
            LISTAC *lala = temp->utilizadores;
            if(procuraID(*lala,temp->n_utilizadores) == 0) {
                //printf("cheguei aqui procuraID() == 0 \n");
                addClient(temp->utilizadores,sock,temp->n_utilizadores,temp);                    //adiciona o utilizador a lista ligada
            }
            else{
                int numero_id = procuraMax(*lala)+1;
                addClient(temp->utilizadores,sock,numero_id,temp);
            }
            
            // temporario server para ter as informacoes do cliente e assim criar o thread
            temporary->c_socket=sock;
            temporary->id=temp->n_utilizadores;
            temporary->next=NULL;
            temporary->sala_atual = temp;
            

            return 1;
        }
        if(temp->next == NULL) return 0;
        temp = temp->next;
    }
}



struct c_info procura_ultimo_user_sala(SALA salita) {
    // procura o ultimo utilizador da sala ( a cauda )
    struct c_info *temp = salita.utilizadores->pfirst;
    while(temp->next != NULL) temp = temp->next;
        return *temp;
}

SALA *procura_id_salas(int id_sala) {
    SALA *temp = salas.pfirst;
    while(1) {
        if(temp == NULL) return NULL;
        if(temp->sala_id == id_sala) return temp;
        if(temp->next == NULL) return NULL;
        temp = temp->next;
    }
    
}



void criarSala(int mod) {
    //cria uma sala e atribui um moderador
    
    //parte clients ---
    LISTAC *lis_temp = malloc(sizeof(LISTAC));
    struct c_info *client = malloc(sizeof(struct c_info));
    client->c_socket=mod;
    client->id=1;
    client->next = NULL;
    lis_temp->pfirst=client;
    //parte servidor ---
    SALA *aux = malloc(sizeof(SALA));
    aux->mod = mod;
    aux->n_utilizadores = 1;
    aux->next = NULL;
    aux->utilizadores = lis_temp;
    aux->sala_id= procura_id_salas_max()+1;
    aux->clients[0] = mod;
    printf("sala criada: \n mod %d \n n_utilizadores %d \n sala_id %d \n",aux->mod,aux->n_utilizadores,aux->sala_id);
    
    if(salas.pfirst == NULL) {
        salas.pfirst = aux;
    } else {
        SALA *temp = salas.pfirst;
        while(1) {
            //DESLOCA-SE ATÉ Á CAUDA
            if(temp->next == NULL) {
                temp->next = aux;
                break;
            }
            temp = temp->next;
        }
    }
    client->sala_atual = aux;
    
    pthread_create(&thread_sala[aux->sala_id-1],NULL,salas_thread,NULL);
    aux->id=thread_sala[aux->sala_id-1];
    // na saaa com o thread itera a sala ate encontrar o id da mesma
    
}

int procura_id_salas_max() {
    if(salas.pfirst == NULL) return 0;
    SALA *aux = salas.pfirst;
    int max = aux->sala_id;
    while(1) {
        if(aux->sala_id > max) max = aux->sala_id;
        if(aux->next == NULL) break;
        aux = aux->next;
    }
    //printf("MAX = %d \n",max);
    return max;
}

int procuraMax(LISTAC lista) {
    if(lista.pfirst == NULL) return 0;
    struct c_info *paux = lista.pfirst;
    int max = paux->id;
    while(1) {
        if(paux->id > max) max = paux->id;
        if(paux->next == NULL) break;
        paux = paux->next;
    }
    //printf("max = %d\n",max);
    return max;
}

int procuraID(LISTAC lista , int id) {
    // ESTA FUNCAO RETURNA O NUMERO DO SOCKET  CASO HAJA UM ID IGUAL NOS UTILIZADORES E 0 CASO NEGATIVO
    
    if(lista.pfirst == NULL) return 0;
    //printf("procuraID(): A PROCURAR PRIMEIRO ID: %d ESTE ID: %d \n",lista.pfirst->id,id);
    struct c_info *paux = lista.pfirst;
    while(1) {
        //printf("ciclo \n");
        if(paux->id == id) {
            //printf("procuraID(): ENCONTREI ID \n");
            return paux->c_socket;
        }
        if(paux->next == NULL) {
            //printf("procuraID(): nao enceontrei este ID \n");
            return 0;
        }
        paux = paux->next;
    }
    //printf("procuraID(): dei return 0 \n");
    return 0;
}

void addClient(LISTAC *lista ,int socket,int id, SALA *sala) {
    struct c_info *novo_cliente = malloc(sizeof(struct c_info));
    novo_cliente->id = id;
    novo_cliente->next = NULL;
    novo_cliente->c_socket = socket;
    novo_cliente->sala_atual = sala;
    
    struct c_info *paux = lista->pfirst;
    if(lista->pfirst == NULL) {
        // caso seja o primeiro elemento a ser adicionado
        lista->pfirst = novo_cliente;
        //printf("addClient(): ID DO PRIMEIRO CLIENTE %d\n",lista->pfirst->id);
        return;
    }
    while(1) {
        //DESLOCA-SE ATÉ Á CAUDA
        if(paux->next == NULL) {
            paux->next = novo_cliente;
            //printf("addClient(): ID DO NOVO CLIENTE %d\n",paux->next->id);
            break;
        }
        paux= paux->next;
    }
    
    return;
}

void rmClient(LISTAC *lista,int id,SALA * sala_a_sair) {
    //elemina no array
    int i,j;
    for(i = 0; i < sala_a_sair->n_utilizadores; i++) {
        if(sala_a_sair->clients[i] == id) {
            j = i;
            while(j < sala_a_sair->n_utilizadores-1) {
                sala_a_sair->clients[j] = sala_a_sair->clients[j+1];
                j++;
            }
        }
    }
    sala_a_sair->n_utilizadores--;
    
    //elemina na lista ligada
    if(lista->pfirst == NULL) return;
    struct c_info *paux = lista->pfirst;
    struct c_info *pprev = NULL;
    while(1) {
        if(paux->id == id) {
            if(paux->id == lista->pfirst->id) {
                // caso seja a cabeca a ser eleminada
                //printf("rmClient(): A ELIMINAR A CAUDA \n");
                lista->pfirst = paux->next;
                free(paux);
                return;
            }
            //printf("rmClient(): CHEUGEI AQUI \n");
            pprev->next = paux->next;
            free(paux);
            break;
        }
        if(paux->next == NULL) {
            //printf("rmClient(): ID NAO ENCONTRADO \n");
            break;
        }
        //printf("rmClient(): um ciclo \n" );
        pprev = paux;
        paux = paux->next;
    }
    return;
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
