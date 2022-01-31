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
    while(read(client_socks[index], &m_client, sizeof(m_client)) == sizeof(m_client)){
        pthread_mutex_lock(&draw_mutex);
        printf("message key:%d\n", m_client.key);
        if(m_client.type==4){
            /* disconnect - remove user from list */
            if (close(client_socks[index]) == -1){
                perror("closing socket");
                exit(-1);
            }
            client_socks[index] = client_socks[Num_players - 1];
            Players_paddle[index] = Players_paddle[Num_players - 1];
            Players_score[index] = Players_score[Num_players - 1];
            Players_paddle[Num_players - 1].length = -1;
            Players_score[Num_players - 1] = 0;
            Num_players--;
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
        ordered_message.paddles[0] = m.paddles[index];
        for (int k = 1; k < Num_players; k++){
            if (k < index)
                ordered_message.paddles[k] = m.paddles[k - 1];
            else if (k > index)
                ordered_message.paddles[k] = m.paddles[k];
            else
                continue;
        }

        for (int k = 0; k < Num_players; k++)
            if (write(client_socks[k], &ordered_message, sizeof(m)) == -1){
                perror("client thread write\n");
                exit(-1);
            }
        

        printf("sent message to client %d\n", index);
        pthread_mutex_unlock(&draw_mutex);
    }
}

void * moveBall(void * arg){
    message_server ordered_message;
    while(1){
        sleep(1);
        pthread_mutex_lock(&draw_mutex);
        
        moove_ball(&m.ball, Players_paddle, Num_players, Players_score);
        for(int aux=0; aux<Num_players; aux++){
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
    for (int a = 0; a < MAX_NUMBER_OF_PLAYERS; a++)
        Players_paddle[a].length = -1;
    
    if(listen(sock_fd, 1) == -1) {
		perror("listen");
		exit(-1);
	}
    pthread_create(&ball_thread, NULL, moveBall, NULL);
    while(1){
		client_socks[Num_players] = accept(sock_fd, NULL, NULL);
		if(client_socks[Num_players] == -1) {
			perror("accept");
			exit(-1);
		}
        printf("accepted new client %d\n", Num_players);
        new_paddle(&Players_paddle[Num_players], PADLE_SIZE, Players_paddle, Num_players, m.ball);
        Players_score[Num_players]=0;

        if(Num_players + 1<MAX_NUMBER_OF_PLAYERS)
            m.paddles[Num_players + 1].length=-1;

        m.paddles[Num_players]=Players_paddle[Num_players];
        m.score = 0;
        write(client_socks[Num_players], &m, sizeof(message_server));
        pthread_create(&client_tids[Num_players], NULL, ClientThreads, (void *) &Num_players);
        printf("thread created\n");
        Num_players++;
    }
    pthread_mutex_destroy(&draw_mutex);
}