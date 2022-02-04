#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

#include "Serverfunc.h"
#include "sock_dg_inet.h"

message_server m;
pthread_mutex_t draw_mutex;
struct paddle_position_t *Players_paddle;
int *Players_score;
int Num_players=0;
int *client_socks;
int *client_index;

void * ClientThreads(void * arg){
    
    int index = *(int *)arg;    
    message_client m_client;
    printf("entrou na thread: %d\n", index);
    while(read(client_socks[index], &m_client, sizeof(m_client)) == sizeof(m_client)){
        printf("entrou no read\n");
        pthread_mutex_lock(&draw_mutex);
        printf("message key:%d\n", m_client.key);
        int index = *(int *)arg;
        if(m_client.type==4){
            /* disconnect - remove user from list */
            client_socks[index] = -1;
            printf("deu disconnect- worth\n");
            Players_score[index] = 0;
            client_index[index] = -1;
            Players_paddle[index].length = -1;
            for (int j = 0; j < MAX_NUMBER_OF_PLAYERS; j++){
                m.paddles[j] = Players_paddle[j];
            }
            Num_players--;
            pthread_mutex_unlock(&draw_mutex);
            return NULL;
        }
        if (m_client.key == KEY_UP || m_client.key == KEY_DOWN || m_client.key == KEY_LEFT || m_client.key == KEY_RIGHT){
            moove_paddle(&Players_paddle[index], Players_paddle, m_client.key, MAX_NUMBER_OF_PLAYERS, index, &m.ball);
            for (int j = 0; j < MAX_NUMBER_OF_PLAYERS; j++){
                m.paddles[j] = Players_paddle[j];
            }   
        }


        for (int k = 0; k < MAX_NUMBER_OF_PLAYERS; k++){
            m.index=k;
            m.score=Players_score[k];
            if (client_socks[k] != -1)
                if (write(client_socks[k], &m, sizeof(m)) == -1){
                    perror("client thread write\n");
                    exit(-1);
                }
        }
        
        printf("sent message to client %d\n", index);
        pthread_mutex_unlock(&draw_mutex);
    }
    return NULL;
}

void * moveBall(void * arg){

    while(1){
        sleep(1);
        pthread_mutex_lock(&draw_mutex);

        moove_ball(&m.ball, Players_paddle, Num_players, Players_score);
        for(int aux=0; aux<MAX_NUMBER_OF_PLAYERS; aux++){  
            m.index=aux;
            m.score=Players_score[aux];
            if (client_socks[aux] != -1 && client_index[aux] != -1){
                printf("Move_ball:  Id:%d\nclient_socks[%d] = %d\n", client_index[aux], aux, client_socks[aux]);
                if (write(client_socks[aux], &m, sizeof(message_server)) == -1){
                    perror("writing on stream socket");
                    exit(-1); 
                }
            }
        }
            
        pthread_mutex_unlock(&draw_mutex);
    }
    return NULL;
}

int main(){
    Players_paddle = malloc(MAX_NUMBER_OF_PLAYERS *sizeof(struct paddle_position_t));
    Players_score = malloc(MAX_NUMBER_OF_PLAYERS * sizeof(int));
    client_index = malloc(MAX_NUMBER_OF_PLAYERS * sizeof(int));
    pthread_t *client_tids = malloc(MAX_NUMBER_OF_PLAYERS * sizeof(pthread_t));
    pthread_t ball_thread;
    client_socks = malloc(MAX_NUMBER_OF_PLAYERS * sizeof(int));
    
    pthread_mutex_init(&draw_mutex, NULL);

    int sock_fd=Socket_creation();
    Socket_identification(sock_fd);
    place_ball_random(&m.ball);
    for (int a = 0; a < MAX_NUMBER_OF_PLAYERS; a++){
        client_socks[a] = -1;
        Players_paddle[a].length = -1;
        client_index[a]=-1;
    }
    
    if(listen(sock_fd, MAX_NUMBER_OF_PLAYERS) == -1) {
		perror("listen");
		exit(-1);
	}
    int x;
    pthread_create(&ball_thread, NULL, moveBall, NULL);
    while(1){
		x = accept(sock_fd, NULL, NULL);
        int help=0;
        while(client_index[help]!=-1){
            help++;
            printf("help = %d\n", help);
        }

        client_socks[help]=x;
		if(x == -1) {
            printf("é isto?\n");
			perror("accept");
			exit(-1);
		}
        printf("SOCK_NUM:%d\n", client_socks[help]);
        printf("accepted new client %d\n", Num_players);
        new_paddle(&Players_paddle[help], PADLE_SIZE, Players_paddle, Num_players, m.ball);
        Players_score[help]=0;

        m.paddles[help]=Players_paddle[help];
        m.score = 0;
        if( write(client_socks[help], &m, sizeof(message_server))==-1){
            perror("main write\n");
            exit(-1);
        }
        
        client_index[help]=help;
        printf("help:%d\nclient_index[help] = %d\n", help, client_index[help]);
        pthread_create(&client_tids[help], NULL, ClientThreads, (void *) &client_index[help]);
        printf("thread created\n");
        Num_players++;
    }
    pthread_mutex_destroy(&draw_mutex);
}