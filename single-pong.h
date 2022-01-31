#include <stdlib.h>
#include <ncurses.h>

#include "pong.h"
#define MAX_NUMBER_OF_PLAYERS 10

WINDOW * message_win;

typedef struct ball_position_t{
    int x, y;
    int up_hor_down; //  -1 up, 0 horizontal, 1 down
    int left_ver_right; //  -1 left, 0 vertical,1 right
    char c;
} ball_position_t;

typedef struct paddle_position_t{
    int x, y;
    int length;
} paddle_position_t;

typedef struct message_server{
    ball_position_t ball;
    paddle_position_t paddles[MAX_NUMBER_OF_PLAYERS];
    int score;
} message_server;

typedef struct message_client {
    //Type: 1 - connect, 2 - paddle_move, 4 - disconnect
    int type;
    int key;
} message_client;


void new_paddle (paddle_position_t *, int, paddle_position_t *, int, ball_position_t);
void draw_paddle(WINDOW *, paddle_position_t *, int, char);
void moove_paddle (paddle_position_t *, paddle_position_t *, int, int, int, ball_position_t *);
void place_ball_random(ball_position_t *);
void moove_ball(ball_position_t *, paddle_position_t *, int, int *);
void draw_ball(WINDOW *, ball_position_t *, int);
void update_board(WINDOW *, message_server, ball_position_t, paddle_position_t *);
void update_ball_on_screen(WINDOW *, ball_position_t *);