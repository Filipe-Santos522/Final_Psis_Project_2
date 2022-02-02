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
    message_server ordered_message;
    printf("entrou na thread: %d\n", index);
    while(read(client_socks[index], &m_client, sizeof(m_client)) == sizeof(m_client)){
        printf("entrou no read\n");
        pthread_mutex_lock(&draw_mutex);
        printf("message key:%d\n", m_client.key);
        int index = *(int *)arg;
        if(m_client.type==4){
            int sock_aux;
            /* disconnect - remove user from list */
            sock_aux= client_socks[index];
            client_socks[index] = client_socks[Num_players - 1];
            client_socks[Num_players - 1]=sock_aux;
            if (close(client_socks[Num_players-1]) == -1){
                perror("closing socket");
                exit(-1);
            }
            printf("deu disconnect- worth\n");
            client_socks[index] = client_socks[Num_players - 1];
            Players_paddle[index] = Players_paddle[Num_players - 1];
            Players_score[index] = Players_score[Num_players - 1];
            client_index[Num_players-1]=index;
            client_index[index]=-1;
            Players_paddle[Num_players - 1].length = -1;
            for (int j = 0; j < Num_players; j++){
                m.paddles[j] = Players_paddle[j];
            }
            Players_score[Num_players - 1] = 0;
            Num_players--;
            pthread_mutex_unlock(&draw_mutex);
            return;
        }
        if (m_client.key == KEY_UP || m_client.key == KEY_DOWN || m_client.key == KEY_LEFT || m_client.key == KEY_RIGHT){
            moove_paddle(&Players_paddle[index], Players_paddle, m_client.key, Num_players, index, &m.ball);
            for (int j = 0; j < Num_players; j++){
                m.paddles[j] = Players_paddle[j];
            }   
        }
        ordered_message.ball = m.ball;
        ordered_message.score = m.score;
        for (int k = 0; k < Num_players; k++){
                ordered_message.paddles[k] = m.paddles[k];
        }

        for (int k = 0; k < Num_players; k++){
            ordered_message.index=k;
            if (write(client_socks[k], &ordered_message, sizeof(m)) == -1){
                perror("client thread write\n");
                exit(-1);
            }
        }
        

        printf("sent message to client %d\n", index);
        pthread_mutex_unlock(&draw_mutex);
    }
}

void * moveBall(void * arg){

    while(1){
        sleep(1);
        pthread_mutex_lock(&draw_mutex);

        moove_ball(&m.ball, Players_paddle, Num_players, Players_score);
        for(int aux=0; aux<Num_players; aux++){
            printf("Move_ball:  Id:%d\n", aux);

            m.index=aux;
            if (write(client_socks[aux], &m, sizeof(message_server)) == -1){
                perror("writing on stream socket");
                exit(-1); 
            }
        }
            
        pthread_mutex_unlock(&draw_mutex);
    }
    return;
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
        Players_paddle[a].length = -1;
        client_index[a]=-1;
    }
    
    if(listen(sock_fd, 10) == -1) {
		perror("listen");
		exit(-1);
	}
    pthread_create(&ball_thread, NULL, moveBall, NULL);
    while(1){
		client_socks[Num_players] = accept(sock_fd, NULL, NULL);
		if(client_socks[Num_players] == -1) {
            printf("é isto?\n");
			perror("accept");
			exit(-1);
		}
        printf("SOCK_NUM:%d\n", client_socks[Num_players]);
        printf("accepted new client %d\n", Num_players);
        new_paddle(&Players_paddle[Num_players], PADLE_SIZE, Players_paddle, Num_players, m.ball);
        Players_score[Num_players]=0;

        if(Num_players + 1<MAX_NUMBER_OF_PLAYERS)
            m.paddles[Num_players + 1].length=-1;

        m.paddles[Num_players]=Players_paddle[Num_players];
        m.score = 0;
        if( write(client_socks[Num_players], &m, sizeof(message_server))==-1){
            perror("client thread write\n");
            exit(-1);
        }
        int help=0;
        while(client_index[help]!=-1){
            help++;
        }
        client_index[help]=Num_players;
        printf("help:%d\n", help);
        pthread_create(&client_tids[Num_players], NULL, ClientThreads, (void *) &client_index[help]);
        printf("thread created\n");
        Num_players++;
    }
    pthread_mutex_destroy(&draw_mutex);
}