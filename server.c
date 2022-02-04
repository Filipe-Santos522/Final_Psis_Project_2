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



/*A thread for each player that receives player imput and responds with board state*/
void * ClientThreads(void * arg){
    
    int index = *(int *)arg;    
    message_client m_client;

    while(read(client_socks[index], &m_client, sizeof(m_client)) == sizeof(m_client)){

        pthread_mutex_lock(&draw_mutex); //prevents changing/accessing the same parameters at the same time as other ClientThreads/Moveball function
        printf("message key:%d\n", m_client.key);
        int index = *(int *)arg;
        if(m_client.type==4){
            /* disconnect - remove user from list -> all "-1" means the paddle is inactive*/
            client_socks[index] = -1;
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
        if (m_client.key == KEY_UP || m_client.key == KEY_DOWN || m_client.key == KEY_LEFT || m_client.key == KEY_RIGHT){ //move paddle from client request
            moove_paddle(&Players_paddle[index], Players_paddle, m_client.key, MAX_NUMBER_OF_PLAYERS, index, &m.ball);
            for (int j = 0; j < MAX_NUMBER_OF_PLAYERS; j++){
                m.paddles[j] = Players_paddle[j];
            }   
        }


        for (int k = 0; k < MAX_NUMBER_OF_PLAYERS; k++){ //send all clients the current board state
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


/*A thread that periodically changes ball position and sends board state to all players*/
void * moveBall(void * arg){

    while(1){
        sleep(1);
        pthread_mutex_lock(&draw_mutex); //prevents changing/accessing the same parameters at the same time as other ClientThreads/Moveball function

        moove_ball(&m.ball, Players_paddle, Num_players, Players_score); //change ball position
        for(int aux=0; aux<MAX_NUMBER_OF_PLAYERS; aux++){   //send to all clients
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

    int sock_fd=Socket_creation(); // Open socket for communication
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
    pthread_create(&ball_thread, NULL, moveBall, NULL); //thread to periodically change ball position
    while(1){
		x = accept(sock_fd, NULL, NULL); //waits for player connects
        int help=0;
        while(client_index[help]!=-1){ //checks for an available spot on the player vector
            help++;
        }

        client_socks[help]=x; //puts the descriptor on the vector
		if(x == -1) {
			perror("accept");
			exit(-1);
		}
        printf("accepted new client %d\n", Num_players);
        new_paddle(&Players_paddle[help], PADLE_SIZE, Players_paddle, Num_players, m.ball);//initiallizes paddle position
        Players_score[help]=0;

        m.paddles[help]=Players_paddle[help];
        m.score = 0;
        if( write(client_socks[help], &m, sizeof(message_server))==-1){ //sends connect message
            perror("main write\n");
            exit(-1);
        }
        
        client_index[help]=help;
        pthread_create(&client_tids[help], NULL, ClientThreads, (void *) &client_index[help]); //create client thread
        printf("thread created\n");
        Num_players++;
    }
    pthread_mutex_destroy(&draw_mutex);
}