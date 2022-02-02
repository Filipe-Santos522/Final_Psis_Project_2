#include <stdlib.h>
#include <ncurses.h>


#include "pong.h"
#include "single-pong.h"


void new_paddle(paddle_position_t * paddle, int legth, paddle_position_t *Player_paddles, int numPlayers, ball_position_t ball){
    int y = 1;
    for (int i = 0; i < numPlayers; i++){
        if (Player_paddles[i].y != y)
            break;
        y++;
    }
    paddle->x = (rand() % WINDOW_SIZE/2) + 3;
    paddle->y = y;
    paddle->length = legth;
}

void draw_paddle(WINDOW *win, paddle_position_t * paddle, int delete, char c){
    int ch;
    if(delete){
        ch = c;
    }else{
        ch = ' ';
    }
    int start_x = paddle->x - paddle->length;
    int end_x = paddle->x + paddle->length;
    for (int x = start_x; x <= end_x; x++){
        wmove(win, paddle->y, x);
        waddch(win,ch);
    }
    wrefresh(win);
}

void moove_paddle (paddle_position_t * paddle, paddle_position_t * paddles, int direction, int numPlayers, int index, ball_position_t *ball){
    int paddle_aux;
    int flag=0;

    if (direction == KEY_UP){
        paddle_aux=paddle->y-1;
        for (int i = 0; i < numPlayers; i++)
            if (paddle_aux == paddles[i].y && (paddles[i].x-paddles[index].x < 5 && paddles[i].x-paddles[index].x > -5) && paddles[i].length > 0)
                flag=1;

        if (paddle->y  != 1 && flag!=1){
            if (ball->y == paddle->y - 1 && ball->y != 1 && (ball->x - paddle->x < 3 && ball->x - paddle->x > -3)){
                ball->y--;
                ball->up_hor_down = -1;
                paddle->y--;
            }
            else if (paddle->y - 1 != ball->y || ball->x - paddle->x >= 3 || ball->x - paddle->x <= -3)
                paddle->y--;
        }
    }


    if (direction == KEY_DOWN){
        paddle_aux=paddle->y+1;
        for (int i = 0; i < numPlayers; i++)
            if (paddle_aux == paddles[i].y && (paddles[i].x-paddles[index].x < 5 && paddles[i].x-paddles[index].x > -5) && paddles[i].length > 0)
                flag=1;

        if (paddle->y  != WINDOW_SIZE - 2 && flag!=1){
            if (ball->y == paddle->y + 1 && ball->y != WINDOW_SIZE - 2 && (ball->x - paddle->x < 3 && ball->x - paddle->x > -3)){
                ball->y++;
                ball->up_hor_down = 1;
                paddle->y++;
            }
            else if (paddle->y + 1 != ball->y || ball->x - paddle->x >= 3 || ball->x - paddle->x <= -3)
                paddle->y++;
        }
        
    }
    
    if (direction == KEY_LEFT){
        paddle_aux=paddle->x-1;
        for (int i = 0; i < numPlayers; i++)
            if (paddle->y == paddles[i].y && (paddles[i].x-paddle_aux < 5 && paddles[i].x-paddle_aux > -5) && paddles[i].length > 0){
                if(i != index)
                    flag=1;
            }


        if (paddle->x  != 3 && flag!=1){
            if (ball->y == paddle->y && ball->x != 1 &&  ball->x - paddle->x == -3){
                ball->x--;
                ball->left_ver_right = -1;
                paddle->x--;
            }
            else if (paddle->y != ball->y || ball->x - paddle->x <= -3)
                paddle->x--;
        }
        
    }
    
    if (direction == KEY_RIGHT){
        paddle_aux=paddle->x+1;
        for (int i = 0; i < numPlayers; i++)
            if (paddle->y == paddles[i].y && (paddles[i].x-paddle_aux < 5 && paddles[i].x-paddle_aux > -5) && paddles[i].length > 0){
                if(i != index)
                    flag=1;
            }
            
        if (paddle->x  != WINDOW_SIZE-4 && flag!=1){
            if (ball->y == paddle->y && ball->x != WINDOW_SIZE-2 && ball->x - paddle->x == 3){
                ball->x++;
                ball->left_ver_right = 1;
                paddle->x++;
            }
            else if (paddle->y != ball->y || ball->x - paddle->x >= 3)
                paddle->x++;
        }
    }
}

void place_ball_random(ball_position_t * ball){
    ball->x = rand() % WINDOW_SIZE ;
    ball->y = rand() % WINDOW_SIZE ;
    ball->c = 'o';
    ball->up_hor_down = rand() % 3 -1; //  -1 up, 1 - down
    ball->left_ver_right = rand() % 3 -1 ; // 0 vertical, -1 left, 1 right
}
void moove_ball(ball_position_t * ball, paddle_position_t *paddles, int numPlayers, int *scores){
    
    int next_x = ball->x + ball->left_ver_right;
    int next_y = ball->y + ball->up_hor_down;

    int flag3=0;
    int flag=0;
    int flag2=0;
    int flag4=0;
    for (int i = 0; i < numPlayers; i++){
        if ((paddles[i].x + 2 >= next_x && paddles[i].x - 2 <= next_x) && paddles[i].y == next_y){
            if(ball->up_hor_down==0){
                flag2=1;
            }else{
                flag = 1; 
            }     
            scores[i]++;
        }
    }

    for (int i = 0; i < numPlayers; i++){
        if (paddles[i].y==ball->y && (paddles[i].x + 2 >= next_x && paddles[i].x - 2 <= next_x)){
            flag3=1;
        }
        if(paddles[i].y==next_y && (paddles[i].x + 2 >= ball->x && paddles[i].x - 2 <= ball->x))
            flag4=1;
    }

    if( (next_y == 0 || next_y == WINDOW_SIZE-1 || flag == 1) && (next_x == 0 || next_x == WINDOW_SIZE-1 || flag2 == 1)){
        ball->up_hor_down *= -1;
        ball->left_ver_right *= -1;
        mvwprintw(message_win, 2,1,"bottom top win");
        wrefresh(message_win);
        return;
    }

    if( next_x == 0 || next_x == WINDOW_SIZE-1 || flag2 == 1 || flag3==1){
        ball->up_hor_down = rand() % 3 -1 ;
        ball->left_ver_right *= -1;
        mvwprintw(message_win, 2,1,"left right win");
        wrefresh(message_win);
     }else{
        ball->x = next_x;
    }
    
    if( next_y == 0 || next_y == WINDOW_SIZE-1 || flag == 1 || flag4==1){
        ball->up_hor_down *= -1;
        ball->left_ver_right = rand() % 3 -1;
        mvwprintw(message_win, 2,1,"bottom top win");
        wrefresh(message_win);
    }else{
        ball -> y = next_y;
    }
}

void draw_ball(WINDOW *win, ball_position_t * ball, int draw){
    int ch;
    if(draw){
        ch = ball->c;
    }else{
        ch = ' ';
    }
    wmove(win, ball->y, ball->x);
    waddch(win,ch);
    wrefresh(win);
}

void update_board(WINDOW* my_win, message_server m, ball_position_t prev_ball, paddle_position_t *prev_paddles){

    draw_ball(my_win, &prev_ball, false);
    draw_ball(my_win, &m.ball, true);

    draw_paddle(my_win, &prev_paddles[0], false, '=');
    draw_paddle(my_win, &m.paddles[0], true, '=');

    int i = 1;
    while (prev_paddles[i].length > 0 && i < MAX_NUMBER_OF_PLAYERS) {
        draw_paddle(my_win, &prev_paddles[i], false, '_');
        i++;
    }
    i = 1;
    while (m.paddles[i].length > 0 && i < MAX_NUMBER_OF_PLAYERS) {
        draw_paddle(my_win, &m.paddles[i], true, '_');
        i++;
    }
/*while (m.paddles[i].length != -1 || i == MAX_NUMBER_OF_PLAYERS) {
        draw_paddle(my_win, &prev_paddles[i], false, '_');
        draw_paddle(my_win, &m.paddles[i], true, '_');
        i++;
    }*/
}


paddle_position_t paddle;
ball_position_t ball;


int teste(){
	initscr();		    	/* Start curses mode 		*/
	cbreak();				/* Line buffering disabled	*/
    keypad(stdscr, TRUE);   /* We get F1, F2 etc..		*/
	noecho();			    /* Don't echo() while we do getch */

    /* creates a window and draws a border */
    WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);	
	wrefresh(my_win);
    keypad(my_win, true);
    /* creates a window and draws a border */
    message_win = newwin(5, WINDOW_SIZE+10, WINDOW_SIZE, 0);
    box(message_win, 0 , 0);	
	wrefresh(message_win);


    //new_paddle(&paddle, PADLE_SIZE);
    //draw_paddle(my_win, &paddle, true, '_');

    //place_ball_random(&ball);
    //draw_ball(my_win, &ball, true);

    int key = -1;
    while(key != 27){
        key = wgetch(my_win);		
        if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN){
            //draw_paddle(my_win, &paddle, false, '_');
            //moove_paddle (&paddle, key);
            //draw_paddle(my_win, &paddle, true, '_');

            //draw_ball(my_win, &ball, false);
            //moove_ball(&ball);
            //draw_ball(my_win, &ball, true);
        }
        mvwprintw(message_win, 1,1,"%c key pressed", key);
        wrefresh(message_win);	
    }

    exit(0);
}