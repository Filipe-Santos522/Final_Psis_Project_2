/* Client source code file by:
 * Filipe Santos - 90068
 * Alexandre Fonseca - 90210

 * Some code provided by the professors was also used
 */

#include <ncurses.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "Serverfunc.h"
#include "sock_dg_inet.h"

int sock_fd;
message_server m_s, prev_message;
pthread_mutex_t draw_mutex;
WINDOW *my_win, *message_win;
int key;

void * updateBoard(void * arg){
    while (read(sock_fd, &m_s, sizeof(m_s)) == sizeof(m_s)){
        pthread_mutex_lock(&draw_mutex);
        draw_ball(my_win, &prev_message.ball, false);
        draw_ball(my_win, &m_s.ball, true);
        for (int j = 0; j < MAX_NUMBER_OF_PLAYERS; j++){
            if (prev_message.paddles[j].length > 0)
                draw_paddle(my_win, &prev_message.paddles[j], false, '_');
        }
        //draw_paddle(my_win, &m_s.paddles[0], true, '=');
        for (int j = 0; j < MAX_NUMBER_OF_PLAYERS; j++){
            if(j!=m_s.index){
                if (m_s.paddles[j].length > 0)
                    draw_paddle(my_win, &m_s.paddles[j], true, '_');
            }else{
                draw_paddle(my_win, &m_s.paddles[j], true, '=');
            }
        }
            
        wrefresh(my_win);
        mvwprintw(message_win, 1,1,"%c key pressed", key); // Print pressed key
        wrefresh(message_win);
        mvwprintw(message_win, 2,1,"Player score: %d", m_s.score); // Print player score
        wrefresh(message_win);
        prev_message.ball = m_s.ball;
        for (int j = 0; j < MAX_NUMBER_OF_PLAYERS; j++)
            prev_message.paddles[j] = m_s.paddles[j];
        prev_message.score = m_s.score;
        pthread_mutex_unlock(&draw_mutex);
    }
}

int main(int argc, char* argv[]){
    m_s.index=0;
    prev_message.index=0;
    // Check terminal arguments
    if(argc!=2){
            printf("Error in arguments\n");
            exit(1);
    }

    // Useful variables initialization
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SOCK_PORT);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    sock_fd=Socket_creation(); // Open socket for communication
    message_client m;
    m.type = 1; /* Set message type to "connect"*/
    m_s.score = 0; /* Set initial score */
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        perror("connect");
        exit(-1);
    }

    pthread_t board_thread;
    pthread_create(&board_thread, NULL, updateBoard, NULL);

    if (read(sock_fd, &m_s, sizeof(m_s)) != sizeof(m_s)){
        perror("first read");
        exit(-1);
    }

    prev_message.ball = m_s.ball;
    for (int j = 0; j < MAX_NUMBER_OF_PLAYERS; j++)
        prev_message.paddles[j] = m_s.paddles[j];
    prev_message.score = m_s.score;

    initscr();		    	/* Start curses mode 		*/
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();			    

    /* creates a window and draws a border */
    my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);	
	wrefresh(my_win);
    keypad(my_win, true);
    /* creates a window and draws a border */
    message_win = newwin(5, WINDOW_SIZE+10, WINDOW_SIZE, 0);
    box(message_win, 0 , 0);	
	wrefresh(message_win);

    pthread_mutex_init(&draw_mutex, NULL);
    
    while (1) {
        
        /*pthread_mutex_lock(&draw_mutex);
        wmove(my_win, 0, 0); // Move cursor to (0,0) (purely aesthetic)
        wrefresh(my_win);
        pthread_mutex_unlock(&draw_mutex);*/
        key = wgetch(my_win); // Get key from user

        // Check which key was pressed (113 is "q" for quit)
        if (key != 113 && (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN)) {
            m.key = key;
            m.type = 2;
            if (write(sock_fd, &m, sizeof(m)) == -1){
                perror("key write");
                exit(-1);
            }
        }
        else {
            m.type = 4;
            m.key = key;
            if (write(sock_fd, &m, sizeof(m)) == -1){
                perror("key write");
                exit(-1);
            }
            break;
        }
    }
    endwin();
    pthread_mutex_destroy(&draw_mutex);
    close(sock_fd);
    exit(0);
}