#include "float.h"
#include <math.h>
#include <algorithm>
#include <stdlib.h>
#include "MyAI.h"
#include "float.h"

#ifdef WINDOWS
#include <time.h>
#else
#include <sys/time.h>
#endif

#define MAX_DEPTH 1
#define WIN 1.0
#define DRAW 0.2
#define LOSE 0.0
#define BONUS 0.3
#define BONUS_MAX_PIECE 8
#define OFFSET (WIN + BONUS)
#define NOEATFLIP_LIMIT 60
#define POSITION_REPETITION_LIMIT 3
#define TRANS_TABLE_SIZE 500000
#define EXACT 1
#define LOWER 2
#define UPPER 3
#define HT_WEIGHT 5
#define LMR_K 4
#define LMR_H 3

typedef unsigned int U32;

int hit_cnt = 0;
int hit_write = 0;
int hit_exact_use = 0;
int total_b = 0;
int total_round = 0;
int HT[10000] = {0};
double sum_t = 0;
double time_limit = 4.5;


const U32 col[4] = {
	0x11111111 , 
	0x22222222 ,
	0x44444444 ,
	0x88888888 ,
};

const U32 row[8] = {
	0x0000000F,
	0x000000F0,
	0x00000F00,
	0x0000F000,
	0x000F0000,
	0x00F00000,
	0x0F000000,
	0xF0000000,
};

const U32 pMoves[32] = {
	0x00000012,
	0x00000025,
	0x0000004A,
	0x00000084,
	0x00000121,
	0x00000252,
	0x000004A4,
	0x00000848,
	0x00001210,
	0x00002520,
	0x00004A40,
	0x00008480,
	0x00012100,
	0x00025200,
	0x0004A400,
	0x00084800,
	0x00121000,
	0x00252000,
	0x004A4000,
	0x00848000,
	0x01210000,
	0x02520000,
	0x04A40000,
	0x08480000,
	0x12100000,
	0x25200000,
	0x4A400000,
	0x84800000,
	0x21000000,
	0x52000000,
	0xA4000000,
	0x48000000,
};

const U32 flip_masks[32] = {
	0x00000116,
	0x0000022D,
	0x0000044B,
	0x00000886,
	0x00001161,
	0x000022D2,
	0x000044B4,
	0x00008868,
	0x00011611,
	0x00022D22,
	0x00044B44,
	0x00088688,
	0x00116110,
	0x0022D220,
	0x0044B440,
	0x00886880,
	0x01161100,
	0x022D2200,
	0x044B4400,
	0x08868800,
	0x11611000,
	0x22D22000,
	0x44B44000,
	0x88688000,
	0x16110000,
	0x2D220000,
	0x4B440000,
	0x86880000,
	0x61100000,
	0xD2200000,
	0xB4400000,
	0x68800000,
};

static unsigned long long hash_table[32][16];

void genHash(){
	srand(1);
	for ( int i = 0;i < 32;++i){
		for ( int j = 0;j < 16;++j){
		hash_table[i][j] = ((unsigned long long)rand() << 32) + rand();
		}
	}
}

// ! An ordering of importance can be
// ! • the one in the PV with the best score,
// ! • the one in the PV causes alpha or beta cutoff,
// ! • the one from null move pruning,
// ! • the one from LMR pruning.
struct transposition{
	unsigned long long position;
	double value;
	int depth = 0;
	int exact = 0;
	unsigned long long p_prime = 0;
	bool taken = false;
};

const int flip_order[32] = {13, 14, 17, 18, 8, 9, 10, 11, 12, 15, 16, 19, 20, 21, 22, 23, 4, 5, 6, 7, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3};

int PVs[20][20] = {
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
};

// transposition *Red_Trans_Table = new transposition[TRANS_TABLE_SIZE];
// transposition *Black_Trans_Table = new transposition[TRANS_TABLE_SIZE];

const int BboardtoIndex[32] = {31,0,1,5,2,16,27,6,3,14,17,19,28,11,7,21,30,4,15,26,13,18,10,20,29,25,12,9,24,8,23,22};

int BitHash(U32 x){ return (x * 0x08ED2BE6) >> 27; }

U32 MSB(unsigned x){
	x|= (x >>16);
	x|= (x >>8);
	x|= (x >>4);
	x|= (x >>2);
	x|= (x >>1);
	return (x>>1)+1;
}

const double mah_dist[32][32] = {
	{0, 1, 2, 3, 1, 1.5, 3, 4, 2, 3, 3.5, 5, 3, 4, 5, 5.5, 4, 5, 6, 7, 5, 6, 7, 8, 6, 7, 8, 9, 7, 8, 9, 10, },
	{1, 0, 1, 2, 1.5, 1, 1.5, 3, 3, 2, 3, 3.5, 4, 3, 4, 5, 5, 4, 5, 6, 6, 5, 6, 7, 7, 6, 7, 8, 8, 7, 8, 9, },
	{2, 1, 0, 1, 3, 1.5, 1, 1.5, 3.5, 3, 2, 3, 5, 4, 3, 4, 6, 5, 4, 5, 7, 6, 5, 6, 8, 7, 6, 7, 9, 8, 7, 8, },
	{3, 2, 1, 0, 4, 3, 1.5, 1, 5, 3.5, 3, 2, 5.5, 5, 4, 3, 7, 6, 5, 4, 8, 7, 6, 5, 9, 8, 7, 6, 10, 9, 8, 7, },
	{1, 1.5, 3, 4, 0, 1, 2, 3, 1, 1.5, 3, 4, 2, 3, 3.5, 5, 3, 4, 5, 5.5, 4, 5, 6, 7, 5, 6, 7, 8, 6, 7, 8, 9, },
	{1.5, 1, 1.5, 3, 1, 0, 1, 2, 1.5, 1, 1.5, 3, 3, 2, 3, 3.5, 4, 3, 4, 5, 5, 4, 5, 6, 6, 5, 6, 7, 7, 6, 7, 8, },
	{3, 1.5, 1, 1.5, 2, 1, 0, 1, 3, 1.5, 1, 1.5, 3.5, 3, 2, 3, 5, 4, 3, 4, 6, 5, 4, 5, 7, 6, 5, 6, 8, 7, 6, 7, },
	{4, 3, 1.5, 1, 3, 2, 1, 0, 4, 3, 1.5, 1, 5, 3.5, 3, 2, 5.5, 5, 4, 3, 7, 6, 5, 4, 8, 7, 6, 5, 9, 8, 7, 6, },
	{2, 3, 3.5, 5, 1, 1.5, 3, 4, 0, 1, 2, 3, 1, 1.5, 3, 4, 2, 3, 3.5, 5, 3, 4, 5, 5.5, 4, 5, 6, 7, 5, 6, 7, 8, },
	{3, 2, 3, 3.5, 1.5, 1, 1.5, 3, 1, 0, 1, 2, 1.5, 1, 1.5, 3, 3, 2, 3, 3.5, 4, 3, 4, 5, 5, 4, 5, 6, 6, 5, 6, 7, },
	{3.5, 3, 2, 3, 3, 1.5, 1, 1.5, 2, 1, 0, 1, 3, 1.5, 1, 1.5, 3.5, 3, 2, 3, 5, 4, 3, 4, 6, 5, 4, 5, 7, 6, 5, 6, },
	{5, 3.5, 3, 2, 4, 3, 1.5, 1, 3, 2, 1, 0, 4, 3, 1.5, 1, 5, 3.5, 3, 2, 5.5, 5, 4, 3, 7, 6, 5, 4, 8, 7, 6, 5, },
	{3, 4, 5, 5.5, 2, 3, 3.5, 5, 1, 1.5, 3, 4, 0, 1, 2, 3, 1, 1.5, 3, 4, 2, 3, 3.5, 5, 3, 4, 5, 5.5, 4, 5, 6, 7, },
	{4, 3, 4, 5, 3, 2, 3, 3.5, 1.5, 1, 1.5, 3, 1, 0, 1, 2, 1.5, 1, 1.5, 3, 3, 2, 3, 3.5, 4, 3, 4, 5, 5, 4, 5, 6, },
	{5, 4, 3, 4, 3.5, 3, 2, 3, 3, 1.5, 1, 1.5, 2, 1, 0, 1, 3, 1.5, 1, 1.5, 3.5, 3, 2, 3, 5, 4, 3, 4, 6, 5, 4, 5, },
	{5.5, 5, 4, 3, 5, 3.5, 3, 2, 4, 3, 1.5, 1, 3, 2, 1, 0, 4, 3, 1.5, 1, 5, 3.5, 3, 2, 5.5, 5, 4, 3, 7, 6, 5, 4, },
	{4, 5, 6, 7, 3, 4, 5, 5.5, 2, 3, 3.5, 5, 1, 1.5, 3, 4, 0, 1, 2, 3, 1, 1.5, 3, 4, 2, 3, 3.5, 5, 3, 4, 5, 5.5, },
	{5, 4, 5, 6, 4, 3, 4, 5, 3, 2, 3, 3.5, 1.5, 1, 1.5, 3, 1, 0, 1, 2, 1.5, 1, 1.5, 3, 3, 2, 3, 3.5, 4, 3, 4, 5, },
	{6, 5, 4, 5, 5, 4, 3, 4, 3.5, 3, 2, 3, 3, 1.5, 1, 1.5, 2, 1, 0, 1, 3, 1.5, 1, 1.5, 3.5, 3, 2, 3, 5, 4, 3, 4, },
	{7, 6, 5, 4, 5.5, 5, 4, 3, 5, 3.5, 3, 2, 4, 3, 1.5, 1, 3, 2, 1, 0, 4, 3, 1.5, 1, 5, 3.5, 3, 2, 5.5, 5, 4, 3, },
	{5, 6, 7, 8, 4, 5, 6, 7, 3, 4, 5, 5.5, 2, 3, 3.5, 5, 1, 1.5, 3, 4, 0, 1, 2, 3, 1, 1.5, 3, 4, 2, 3, 3.5, 5, },
	{6, 5, 6, 7, 5, 4, 5, 6, 4, 3, 4, 5, 3, 2, 3, 3.5, 1.5, 1, 1.5, 3, 1, 0, 1, 2, 1.5, 1, 1.5, 3, 3, 2, 3, 3.5, },
	{7, 6, 5, 6, 6, 5, 4, 5, 5, 4, 3, 4, 3.5, 3, 2, 3, 3, 1.5, 1, 1.5, 2, 1, 0, 1, 3, 1.5, 1, 1.5, 3.5, 3, 2, 3, },
	{8, 7, 6, 5, 7, 6, 5, 4, 5.5, 5, 4, 3, 5, 3.5, 3, 2, 4, 3, 1.5, 1, 3, 2, 1, 0, 4, 3, 1.5, 1, 5, 3.5, 3, 2, },
	{6, 7, 8, 9, 5, 6, 7, 8, 4, 5, 6, 7, 3, 4, 5, 5.5, 2, 3, 3.5, 5, 1, 1.5, 3, 4, 0, 1, 2, 3, 1, 1.5, 3, 4, },
	{7, 6, 7, 8, 6, 5, 6, 7, 5, 4, 5, 6, 4, 3, 4, 5, 3, 2, 3, 3.5, 1.5, 1, 1.5, 3, 1, 0, 1, 2, 1.5, 1, 1.5, 3, },
	{8, 7, 6, 7, 7, 6, 5, 6, 6, 5, 4, 5, 5, 4, 3, 4, 3.5, 3, 2, 3, 3, 1.5, 1, 1.5, 2, 1, 0, 1, 3, 1.5, 1, 1.5, },
	{9, 8, 7, 6, 8, 7, 6, 5, 7, 6, 5, 4, 5.5, 5, 4, 3, 5, 3.5, 3, 2, 4, 3, 1.5, 1, 3, 2, 1, 0, 4, 3, 1.5, 1, },
	{7, 8, 9, 10, 6, 7, 8, 9, 5, 6, 7, 8, 4, 5, 6, 7, 3, 4, 5, 5.5, 2, 3, 3.5, 5, 1, 1.5, 3, 4, 0, 1, 2, 3, },
	{8, 7, 8, 9, 7, 6, 7, 8, 6, 5, 6, 7, 5, 4, 5, 6, 4, 3, 4, 5, 3, 2, 3, 3.5, 1.5, 1, 1.5, 3, 1, 0, 1, 2, },
	{9, 8, 7, 8, 8, 7, 6, 7, 7, 6, 5, 6, 6, 5, 4, 5, 5, 4, 3, 4, 3.5, 3, 2, 3, 3, 1.5, 1, 1.5, 2, 1, 0, 1, },
	{10, 9, 8, 7, 9, 8, 7, 6, 8, 7, 6, 5, 7, 6, 5, 4, 5.5, 5, 4, 3, 5, 3.5, 3, 2, 4, 3, 1.5, 1, 3, 2, 1, 0, },
};

// const double bianyuan_system[32] = {
// 	1.5, 1.2, 1.2, 1.5, 
// 	1.2, 1.1, 1.1, 1.2,
// 	1.1, 1, 1, 1.1,
// 	1.1, 1, 1, 1.1,
// 	1.1, 1, 1, 1.1,
// 	1.1, 1, 1, 1.1,
// 	1.2, 1.1, 1.1, 1.2,
// 	1.5, 1.2, 1.2, 1.5, 
// };

MyAI::MyAI(void){srand(time(NULL));}

MyAI::~MyAI(void){}

bool MyAI::protocol_version(const char* data[], char* response){
	strcpy(response, "1.1.0");
  return 0;
}

bool MyAI::name(const char* data[], char* response){
	strcpy(response, "MyAI");
	return 0;
}

bool MyAI::version(const char* data[], char* response){
	strcpy(response, "1.0.0");
	return 0;
}

bool MyAI::known_command(const char* data[], char* response){
  for(int i = 0;i < COMMAND_NUM;i++){
		if(!strcmp(data[0], commands_name[i])){
			strcpy(response, "true");
			return 0;
		}
	}
	strcpy(response, "false");
	return 0;
}

bool MyAI::list_commands(const char* data[], char* response){
  for(int i = 0;i < COMMAND_NUM;i++){
		strcat(response, commands_name[i]);
		if(i < COMMAND_NUM - 1){
			strcat(response, "\n");
		}
	}
	return 0;
}

bool MyAI::quit(const char* data[], char* response){
  fprintf(stderr, "Bye\n");
	return 0;
}

bool MyAI::boardsize(const char* data[], char* response){
  fprintf(stderr, "BoardSize: %s x %s\n", data[0], data[1]);
	return 0;
}

bool MyAI::reset_board(const char* data[], char* response){
	this->Red_Time = -1; // unknown
	this->Black_Time = -1; // unknown
	this->initBoardState();
	return 0;
}

bool MyAI::num_repetition(const char* data[], char* response){
  return 0;
}

bool MyAI::num_moves_to_draw(const char* data[], char* response){
  return 0;
}

bool MyAI::move(const char* data[], char* response){
  char move[6];
	sprintf(move, "%s-%s", data[0], data[1]);
	this->MakeMove(&(this->main_chessboard), move);
	return 0;
}

bool MyAI::flip(const char* data[], char* response){
  char move[6];
	sprintf(move, "%s(%s)", data[0], data[1]);
	this->MakeMove(&(this->main_chessboard), move);
	return 0;
}

bool MyAI::genmove(const char* data[], char* response){
	// set color
	if(!strcmp(data[0], "red")){
		this->Color = RED;
	}else if(!strcmp(data[0], "black")){
		this->Color = BLACK;
	}else{
		this->Color = 2;
	}
	// genmove
  	char move[6];
	this->generateMove(move);
	sprintf(response, "%c%c %c%c", move[0], move[1], move[3], move[4]);
	return 0;
}

bool MyAI::game_over(const char* data[], char* response){
  fprintf(stderr, "Game Result: %s\n", data[0]);
	return 0;
}

bool MyAI::ready(const char* data[], char* response){
  return 0;
}

bool MyAI::time_settings(const char* data[], char* response){
  return 0;
}

bool MyAI::time_left(const char* data[], char* response){
  if(!strcmp(data[0], "red")){
		sscanf(data[1], "%d", &(this->Red_Time));
	}else{
		sscanf(data[1], "%d", &(this->Black_Time));
	}
	fprintf(stderr, "Time Left(%s): %s\n", data[0], data[1]);
	return 0;
}

bool MyAI::showboard(const char* data[], char* response){
  Print_Chessboard();
	return 0;
}

bool MyAI::init_board(const char* data[], char* response){
  initBoardState(data);
	return 0;
}

bool MyAI::isTimeUp(double time_limit){
	double elapsed; // ms
	
	// design for different os
#ifdef WINDOWS
	clock_t end = clock();
	elapsed = (end - begin);
#else
	struct timeval end;
	gettimeofday(&end, 0);
	long seconds = end.tv_sec - begin.tv_sec;
	long microseconds = end.tv_usec - begin.tv_usec;
	elapsed = (seconds*1000 + microseconds*1e-3);
#endif

	return elapsed >= time_limit*1000;
}

// *********************** AI FUNCTION *********************** //

int MyAI::GetFin(char c)
{
	static const char skind[]={'-','K','G','M','R','N','C','P','X','k','g','m','r','n','c','p'};
	for(int f=0;f<16;f++)if(c==skind[f])return f;
	return -1;
}

int MyAI::ConvertChessNo(int input)
{
	switch(input)
	{
	case 0:
		return CHESS_EMPTY;
		break;
	case 8:
		return CHESS_COVER;
		break;
	case 1:
		return 6;
		break;
	case 2:
		return 5;
		break;
	case 3:
		return 4;
		break;
	case 4:
		return 3;
		break;
	case 5:
		return 2;
		break;
	case 6:
		return 1;
		break;
	case 7:
		return 0;
		break;
	case 9:
		return 13;
		break;
	case 10:
		return 12;
		break;
	case 11:
		return 11;
		break;
	case 12:
		return 10;
		break;
	case 13:
		return 9;
		break;
	case 14:
		return 8;
		break;
	case 15:
		return 7;
		break;
	}
	return -1;
}

void MyAI::initBoardState()
{	
	int iPieceCount[14] = {5, 2, 2, 2, 2, 2, 1, 5, 2, 2, 2, 2, 2, 1};
	memcpy(main_chessboard.CoverChess, iPieceCount, sizeof(int)*14);
	main_chessboard.Red_Chess_Num = 16;
	main_chessboard.Black_Chess_Num = 16;
	main_chessboard.NoEatFlip = 0;
	main_chessboard.HistoryCount = 0;

	// convert to my format
	int Index = 0;
	for(int i = 0;i < 8;i++){
		for(int j = 0;j < 4;j++){
			main_chessboard.Board[Index] = CHESS_COVER;
			Index++;
		}
	}
	Print_Chessboard();
	
	// For Zobrist
	genHash();
    for (int i = 0;i < 32;i++){
		int ch = main_chessboard.Board[i];
		if(ch == CHESS_COVER) { ch = 14;}
		if(ch == CHESS_EMPTY) { ch = 15;}
		main_chessboard.hash ^= hash_table[i][ch];
    }
}

void MyAI::initBoardState(const char* data[])
{	
	memset(main_chessboard.CoverChess, 0, sizeof(int)*14);
	main_chessboard.Red_Chess_Num = 0;
	main_chessboard.Black_Chess_Num = 0;
	main_chessboard.NoEatFlip = 0;
	main_chessboard.HistoryCount = 0;

	// set board
	int Index = 0;
	for(int i = 0;i < 8;i++){
		for(int j = 0;j < 4;j++){
			// convert to my format
			int chess = ConvertChessNo(GetFin(data[Index][0]));
			main_chessboard.Board[Index] = chess;
			if(chess != CHESS_COVER && chess != CHESS_EMPTY){
				main_chessboard.CoverChess[chess]--;
				(chess < 7 ? 
					main_chessboard.Red_Chess_Num : main_chessboard.Black_Chess_Num)++;
			}
			Index++;
		}
	}
	// set covered chess
	for(int i = 0;i < 14;i++){
		main_chessboard.CoverChess[i] += data[32 + i][0] - '0';
		(i < 7 ? 
			main_chessboard.Red_Chess_Num : main_chessboard.Black_Chess_Num)
			+= main_chessboard.CoverChess[i];
	}

	Print_Chessboard();
}

void MyAI::generateMove(char move[6])
{
#ifdef WINDOWS
	begin = clock();
#else
	gettimeofday(&begin, 0);
#endif
	int StartPoint = 0, EndPoint = 0;
	double alpha = -DBL_MAX, beta = DBL_MAX;
	this->node = 0;
	int best_move;
	int INIT_SEARCH_DEPTH = 2;
	double best = Nega_Scout(this->main_chessboard, &best_move, this->Color, alpha, beta, 0, INIT_SEARCH_DEPTH, false, INIT_SEARCH_DEPTH, false, false, false);
	
	StartPoint = best_move/100;
	EndPoint = best_move%100;
	sprintf(move, "%c%c-%c%c",'a'+(StartPoint%4),'1'+(7-StartPoint/4),'a'+(EndPoint%4),'1'+(7-EndPoint/4));
	fprintf(stderr, "Depth: %2d, Node: %10d, Score: %+1.5lf, Move: %s\n", INIT_SEARCH_DEPTH, node, best, move);
	fflush(stderr);
	
	// count how many pieces 
	int p_cnt = this->main_chessboard.Black_Chess_Num+this->main_chessboard.Red_Chess_Num;
		
	int SEARCH_DEPTH_LIMIT = (p_cnt >= 20) ? 8 : (p_cnt >= 8) ? 10 : 12;

	int current_depth_limit = 4;
	double threshold = 1000;
	// * IDAS
	while(current_depth_limit <= SEARCH_DEPTH_LIMIT && !isTimeUp(time_limit)){
		double m = Nega_Scout(this->main_chessboard, &best_move, this->Color, best-threshold, best+threshold, 0, current_depth_limit, false, current_depth_limit, false, false, false);
		if(m <= best-threshold){
			m = Nega_Scout(this->main_chessboard, &best_move, this->Color, -DBL_MAX, m, 0, current_depth_limit, false, current_depth_limit, false, false, false);
		}
		else if(m >= best+threshold){
			m = Nega_Scout(this->main_chessboard, &best_move, this->Color, m, DBL_MAX, 0, current_depth_limit, false, current_depth_limit, false, false, false);
		}
		if(!isTimeUp(time_limit)){
			best = m;
			StartPoint = best_move/100;
			EndPoint = best_move%100;
		}
		sprintf(move, "%c%c-%c%c",'a'+(StartPoint%4),'1'+(7-StartPoint/4),'a'+(EndPoint%4),'1'+(7-EndPoint/4));
		fprintf(stderr, "Depth: %2d, Node: %10d, Score: %+1.5lf, Move: %s\n", current_depth_limit, node, best, move);
		
		fflush(stderr);

		current_depth_limit += 2;

	}
	
	// fprintf(stderr, "Hit_cnt: %d, Hit_write: %d, Hit_exact_use: %d\n", hit_cnt, hit_write, hit_exact_use);
	// fprintf(stderr, "total b = %d, total round = %d, average b now = %f\n", total_b, total_round, double(total_b)/double(total_round));

	fprintf(stderr, "\n");
	// * aging the HT table
	// for(int i = 0;i < 1024;i++){
	// 	HT[i] >>= 1;
	// }
	
	char chess_Start[4]="";
	char chess_End[4]="";
	Print_Chess(main_chessboard.Board[StartPoint],chess_Start);
	Print_Chess(main_chessboard.Board[EndPoint],chess_End);
	printf("My result: \n--------------------------\n");
	printf("Nega Scout score max: %lf (node: %d)\n", best, this->node);
	printf("(%d) -> (%d)\n",StartPoint,EndPoint);
	printf("<%s> -> <%s>\n",chess_Start,chess_End);
	printf("move:%s\n",move);
	printf("--------------------------\n");
	this->Print_Chessboard();
}

void MyAI::MakeMove(ChessBoard* chessboard, const int move, const int chess){
	int src = move/100, dst = move%100;
	// FLIP
	if(src == dst){
		chessboard->Board[src] = chess;
		chessboard->CoverChess[chess]--;
		chessboard->NoEatFlip = 0;
		chessboard->Open_Cnt[chess]++;
		chessboard->piece[chess] |= (1 << dst);
		chessboard->occupied |= (1 << dst);
		if(chess/7 == 0){ chessboard->red |= (1<<dst);}
		else{ chessboard->black |= (1<<dst);}
		// HASH
		chessboard->hash ^= hash_table[src][HASH_CHESS_COVER];
		chessboard->hash ^= hash_table[src][chess];

	}
	// MOVE
	else{
		int to_eat_chess = chessboard->Board[src];
		int eaten_chess = chessboard->Board[dst];
		// CAPTURE
		if(eaten_chess != CHESS_EMPTY){
			if(eaten_chess / 7 == 0){ // red
				chessboard->Red_Chess_Num--;
				chessboard->red &= (~(1<<dst));
			}
			else{ // black	
				chessboard->black &= (~(1<<dst));
				chessboard->Black_Chess_Num--;
			}
			chessboard->NoEatFlip = 0;
			
			int chess_no = chessboard->Board[dst] % 7;
			int color = chessboard->Board[dst] / 7;

			for(int i = 0;i < chess_no;i++){
				if(chess_no == 1) break;
				chessboard->Enemy_Cnt[(color^1)*7 + i]--;
			}
			chessboard->Open_Cnt[eaten_chess]--;
			chessboard->Chess_Cnt[eaten_chess]--;
			if(chessboard->Open_Cnt[color*7 + chess_no] == 0){
				chessboard->Enemy_Cnt[(color^1)*7 + chess_no]--;
			}
			
			chessboard->piece[eaten_chess] &= (~(1<<dst));
			chessboard->all_chess &= (~(1<<dst));

			// * HASH
			chessboard->hash ^= hash_table[to_eat_chess][src];
			chessboard->hash ^= hash_table[to_eat_chess][dst];
			chessboard->hash ^= hash_table[eaten_chess][dst];


		}
		// WALK
		else{
			chessboard->NoEatFlip += 1;
			// HASH (chess empty先不管)
			// * HASH
			chessboard->hash ^= hash_table[to_eat_chess][src];
			chessboard->hash ^= hash_table[to_eat_chess][dst];
		}
		
		
		if(to_eat_chess/7 == 0){
  			chessboard->red &= (~(1<<src));
			chessboard->red |= (1<<dst);
		}
		else{
			chessboard->black |= (1<<dst);
  			chessboard->black &= (~(1<<src));
		}
		
		chessboard->occupied |= (1 << dst);
  		chessboard->occupied &= (~(1<<src));

		chessboard->all_chess |= (1 << dst);
		chessboard->all_chess &= (~(1<<src));

		chessboard->piece[to_eat_chess] &= (~(1 << src));	// src piece to dst position!
		chessboard->piece[to_eat_chess] |= (1 << dst);		// src piece to dst position!
		
		chessboard->Board[dst] = chessboard->Board[src];
		chessboard->Board[src] = CHESS_EMPTY;
	}
	chessboard->History[chessboard->HistoryCount++] = move;
}

void MyAI::MakeMove(ChessBoard* chessboard, const char move[6])
{ 
	int src, dst, m;
	src = ('8'-move[1])*4+(move[0]-'a');
	if(move[2]=='('){ // flip
		m = src*100 + src;
		printf("# call flip(): flip(%d,%d) = %d\n",src, src, GetFin(move[3]));
	}else { // move
		dst = ('8'-move[4])*4+(move[3]-'a');
		m = src*100 + dst;
		printf("# call move(): move : %d-%d \n", src, dst);
	}
	MakeMove(chessboard, m, ConvertChessNo(GetFin(move[3])));
	Print_Chessboard();
}

int MyAI::Expand(const int* board, const int color,int *Result, const U32 *piece, const U32 all_chess, const U32 occupied, const U32 red, const U32 black)
{
	int ResultCount = 0;
	if(color != 1 && color != 0 ){
		return ResultCount;
	}
	// opp's pawn
	int opp = (color^1)*7;
	int order[7] = {6, 5, 0, 1, 4, 3, 2};
	if(piece[(this->Color^1)*7+6] == 0){
		order[0] = 6;
		order[1] = 5;
		order[2] = 4;
		order[3] = 3;
		order[4] = 2;
		order[5] = 1;
		order[6] = 0;
	}

	U32 e_pos = (opp == 0) ? red : black;
	
	for(int j = 0; j < 7; j++){
		int i = color*7+order[j];

		U32 p = piece[i];
		// Gun
		if(i == color*7 + 1){
			while(p){
				U32 mask = p & (-p);
				p ^= mask;
				int src = BboardtoIndex[BitHash(mask)];
				int r = src/4;
				int c = src%4;
				// Get the position of cannon in the row of my cannon
				U32 x = ( (row[r] & all_chess) ^ (1 << src) ) >> (4*r);
				// Get the position of cannon in the column of my cannon
				U32 y = ( (col[c] & all_chess) ^ (1 << src) ) >> c;
				// row
				if(c == 0){
					U32 tmp = 0;
					if(x){
						U32 mask2 = x & (-x);
						if((x^mask2) !=0 ){
							x ^= mask2;
							tmp = x & (-x); //LSB
							tmp = tmp << (4*r); // Move back the tmp to the row
							tmp = tmp & e_pos; // Check if its opponent's

							if(tmp != 0){
								int result = BboardtoIndex[BitHash(tmp)];
								Result[ResultCount++] = src*100 + result;	
							}
						}
					}
				}
				else if(c == 1){
					if((x&12)==12){
						// Only check the column of 8
						U32 tmp = 8 << (4*r);
						tmp = tmp & e_pos;
						if(tmp != 0){
							int result= BboardtoIndex[BitHash(tmp)]; //back to original row	
							Result[ResultCount++] = src*100 + result;	
						}
					}
				}
				else if(c == 2){
					if((x&3)==3){
						// Only check the column of 1
						U32 tmp = 1 << (4*r);
						tmp = tmp & e_pos;
						if(tmp != 0){
							int result= BboardtoIndex[BitHash(tmp)];
							Result[ResultCount++] = src*100 + result;	
						}
					}
				}
				else if(c == 3){
					U32 tmp = 0;
					if(x){
						U32 mask2 = MSB(x);
						if((x^mask2) !=0 ){
							x ^= mask2;
							tmp = MSB(x);
							tmp = tmp << (4*r);
							tmp = tmp & e_pos;
							if(tmp != 0){
								int result = BboardtoIndex[BitHash(tmp)];
								Result[ResultCount++] = src*100 + result;	
							}
						}
					}
				}
				U32 tmp = 0;
				if(r == 0){
					if(y){
						U32 mask2 = y & (-y);
						if((y^mask2) !=0 ){
							y ^= mask2;
							tmp = y & (-y);
							tmp = tmp << c;
							tmp = tmp & e_pos;
							if(tmp != 0){
								int result = BboardtoIndex[BitHash(tmp)];
								Result[ResultCount++] = src*100 + result;	
							}
						}
					}
				}
				else if(r == 1){
					y = y & 0x11111100;
					if(y){
						U32 mask2 = y & (-y);
						if((y^mask2) !=0 ){
							y^= mask2;
							tmp = y & (-y); //LSB
							tmp = tmp << c;
							tmp = tmp & e_pos;
							if(tmp != 0){
								int result = BboardtoIndex[BitHash(tmp)];
								Result[ResultCount++] = src*100 + result;	
							}
						}
					}
				}
				else if(r == 2){
					//upper
					if((y&0x00000011) == 0x00000011){
						if( ((1<< c) & e_pos) != 0){
							int result = BboardtoIndex[BitHash(1<<c)];
						       	Result[ResultCount++] = src*100 + result;
						}
					}
					//lower
					y = y & 0x11111000;
					if(y){
						U32 mask2 = y & (-y);
						if((y^mask2) != 0){
							y^= mask2;
							tmp = y & (-y); //LSB
							tmp = tmp << c;
							tmp = tmp & e_pos;
							if(tmp != 0){
								int result = BboardtoIndex[BitHash(tmp)];
								Result[ResultCount++] = src*100 + result;	
							}
						}
					}
				}
				else if(r == 3){
					//upper 
					U32 yu = y & 0x11110000;
					if(yu){
						U32 mask2 = yu & (-yu);
						if((yu^mask2) !=0 ){
							yu^= mask2;
							tmp = yu & (-yu); //dst_lsb
							tmp = tmp << c;
							tmp = tmp & e_pos;
							if(tmp != 0){
								int result = BboardtoIndex[BitHash(tmp)];
								Result[ResultCount++] = src*100 + result;	
							}
						}
					}

					//lower
					U32 yl = y & 0x00000111;
					if(yl){
						U32 mask2 = MSB(yl);
						if((yl^mask2) !=0 ){
							yl^= mask2;
							tmp = MSB(yl); //msb
							tmp = tmp << c;
							tmp = tmp & e_pos;
							if(tmp != 0){
								int result = BboardtoIndex[BitHash(tmp)];
								Result[ResultCount++] = src*100 + result;	
							}
						}
					}
				}
				else if(r == 4){
					//upper 
					U32 yu = y & 0x11100000;
					if(yu){
						U32 mask2 = yu & (-yu);
						if((yu^mask2) !=0 ){
							yu^= mask2;
							tmp = yu & (-yu); //dst_lsb
							tmp = tmp << c;
							tmp = tmp & e_pos;
							if(tmp != 0){
								int result = BboardtoIndex[BitHash(tmp)];
								Result[ResultCount++] = src*100 + result;	
							}
						}
					}
					//lower
					U32 yl = y & 0x00001111;
					if(yl){
						U32 mask2 = MSB(yl);
						if((yl^mask2) !=0 ){
							yl^= mask2;
							tmp = MSB(yl); //msb
							tmp = tmp << c;
							tmp = tmp & e_pos;
							if(tmp != 0){
								int result = BboardtoIndex[BitHash(tmp)];
								Result[ResultCount++] = src*100 + result;	
							}
						}
					}
				}
				else if(r == 5){
					//lower
					if((y&0x11000000) == 0x11000000){
						if( ( ((0x10000000)<< c) & e_pos) != 0){
							int result = BboardtoIndex[BitHash((0x10000000)<<c)];
							Result[ResultCount++] = src*100 + result;
						}
					}	
					//upper
					y = y & 0x00011111;
					if(y){
						U32 mask2 = MSB(y);
						if((y^mask2) !=0 ){
							y^= mask2;
							tmp = MSB(y); //MSB
							tmp = tmp << c;
							tmp = tmp & e_pos;

							if(tmp != 0){
								int result = BboardtoIndex[BitHash(tmp)];
								Result[ResultCount++] = src*100 + result;	
							}
						}
					}
				}
				else if(r == 6){
					y = y & 0x00111111;
					if(y){
						U32 mask2 = MSB(y);
						if((y^mask2) !=0 ){
							y^= mask2;
							tmp = MSB(y);
							tmp = tmp << c;
							tmp = tmp & e_pos;

							if(tmp != 0){
								int result = BboardtoIndex[BitHash(tmp)];
								Result[ResultCount++] = src*100 + result;	
							}
						}
					}
				}
				else if(r == 7){
					if(y){
						U32 mask2 = MSB(y);
						if((y^mask2) !=0 ){
							y^= mask2;
							tmp = MSB(y);
							tmp = tmp << c;
							tmp = tmp & e_pos;

							if(tmp != 0){
								int result = BboardtoIndex[BitHash(tmp)];
								Result[ResultCount++] = src*100 + result;	
							}
						}
					}
				}
			}
		}
		// Not Gun
		else{
			while(p){
				U32 mask = p & (-p);
				p ^= mask;
				int src = BboardtoIndex[BitHash(mask)];
				U32 dst = 0;
				if(i % 7 == 0){
					dst = pMoves[src] & (piece[opp] | piece[opp+6]);
				}
				else if(i%7 == 2){
					dst = pMoves[src] & (piece[opp] | piece[opp+1] | piece[opp+2]);
				}
				else if(i%7 == 3){
					dst = pMoves[src] & (piece[opp] | piece[opp+1] | piece[opp+2] | piece[opp+3]);
				}
				else if(i%7 == 4){
					dst = pMoves[src] & (piece[opp] |  piece[opp+1] |piece[opp+2] | piece[opp+3] | piece[opp+4]);
				}
				else if(i%7 == 5){
					dst = pMoves[src] & (piece[opp] |  piece[opp+1] | piece[opp+2] | piece[opp+3] | piece[opp+4] | piece[opp+5]);
				}
				else if(i%7 == 6){
					dst = pMoves[src] & (piece[opp+6] | piece[opp+1] | piece[opp+2] | piece[opp+3] | piece[opp+4] | piece[opp+5]);
				}

				while(dst){
					U32 dst_lsb = dst & (-dst); // Find LSB
					int dst_idx = BboardtoIndex[BitHash(dst_lsb)];
					Result[ResultCount++] = src*100 + dst_idx;
					dst ^= dst_lsb; // Remove LSB
				}
			}
		}
	}

	// FOR empty move
	for(int i= color*7+6;i >= color*7;i--){
		U32 p = piece[i];
		while(p){
			U32 mask = p & (-p);
			p ^= mask;
			int src = BboardtoIndex[BitHash(mask)];
			U32 dst = 0;
			dst = pMoves[src] & (~all_chess);

			while(dst){
				U32 dst_lsb = dst & (-dst);
				int dst_idx = BboardtoIndex[BitHash(dst_lsb)];
				Result[ResultCount++] = src*100 + dst_idx;
				dst ^= dst_lsb;
			}
		}
	}
	return ResultCount;
}

bool MyAI::Referee(const int* chess, const int from_location_no, const int to_location_no, const int UserId)
{
	// int MessageNo = 0;
	bool IsCurrent = true;
	int from_chess_no = chess[from_location_no];
	int to_chess_no = chess[to_location_no];
	int from_row = from_location_no / 4;
	int to_row = to_location_no / 4;
	int from_col = from_location_no % 4;
	int to_col = to_location_no % 4;

	if(from_chess_no < 0 || ( to_chess_no < 0 && to_chess_no != CHESS_EMPTY) )
	{  
		// MessageNo = 1;
		//strcat(Message,"**no chess can move**");
		//strcat(Message,"**can't move darkchess**");
		IsCurrent = false;
	}
	else if (from_chess_no >= 0 && from_chess_no / 7 != UserId)
	{
		// MessageNo = 2;
		//strcat(Message,"**not my chess**");
		IsCurrent = false;
	}
	else if((from_chess_no / 7 == to_chess_no / 7) && to_chess_no >= 0)
	{
		// MessageNo = 3;
		//strcat(Message,"**can't eat my self**");
		IsCurrent = false;
	}
	//check attack
	else if(to_chess_no == CHESS_EMPTY && abs(from_row-to_row) + abs(from_col-to_col)  == 1)//legal move
	{
		IsCurrent = true;
	}	
	else if(from_chess_no % 7 == 1 ) //judge gun
	{
		int row_gap = from_row-to_row;
		int col_gap = from_col-to_col;
		int between_Count = 0;
		//slant
		if(from_row-to_row == 0 || from_col-to_col == 0)
		{
			//row
			if(row_gap == 0) 
			{
				for(int i=1;i<abs(col_gap);i++)
				{
					int between_chess;
					if(col_gap>0)
						between_chess = chess[from_location_no-i];
					else
						between_chess = chess[from_location_no+i];
					if(between_chess != CHESS_EMPTY)
						between_Count++;
				}
			}
			//column
			else
			{
				for(int i=1;i<abs(row_gap);i++)
				{
					int between_chess;
					if(row_gap > 0)
						between_chess = chess[from_location_no-4*i];
					else
						between_chess = chess[from_location_no+4*i];
					if(between_chess != CHESS_EMPTY)
						between_Count++;
				}
				
			}
			
			if(between_Count != 1 )
			{
				// MessageNo = 4;
				//strcat(Message,"**gun can't eat opp without between one piece**");
				IsCurrent = false;
			}
			else if(to_chess_no == CHESS_EMPTY)
			{
				// MessageNo = 5;
				//strcat(Message,"**gun can't eat opp without between one piece**");
				IsCurrent = false;
			}
		}
		//slide
		else
		{
			// MessageNo = 6;
			//strcat(Message,"**cant slide**");
			IsCurrent = false;
		}
	}
	else // non gun
	{
		//judge pawn or king

		//distance
		if( abs(from_row-to_row) + abs(from_col-to_col)  > 1)
		{
			// MessageNo = 7;
			//strcat(Message,"**cant eat**");
			IsCurrent = false;
		}
		//judge pawn
		else if(from_chess_no % 7 == 0)
		{
			if(to_chess_no % 7 != 0 && to_chess_no % 7 != 6)
			{
				// MessageNo = 8;
				//strcat(Message,"**pawn only eat pawn and king**");
				IsCurrent = false;
			}
		}
		//judge king
		else if(from_chess_no % 7 == 6 && to_chess_no % 7 == 0)
		{
			// MessageNo = 9;
			//strcat(Message,"**king can't eat pawn**");
			IsCurrent = false;
		}
		else if(from_chess_no % 7 < to_chess_no% 7)
		{
			// MessageNo = 10;
			//strcat(Message,"**cant eat**");
			IsCurrent = false;
		}
	}
	return IsCurrent;
}

double MyAI::Evaluate(const ChessBoard* chessboard, const int legal_move_count, const int color, const int depth){
	// score = My Score - Opponent's Score
	// offset = <WIN + BONUS> to let score always not less than zero
	double score = 0;
	// Lose
	if(legal_move_count == 0 && color == this->Color){
		return -1000;
	}
	// Win
	else if(legal_move_count == 0 && color != this->Color){
		return 1000-depth;
	}
	// TODO: Set better or dynamic points for Draw
	else if(isDraw(chessboard)||myRepetitionDraw(chessboard)){ // Draw
		return -800;
	}
	// No Conclusion
	else{
		const double basic_values[14] = {
			1,180,  6, 18, 90,270,810,  
			1,180,  6, 18, 90,270,810
		};

		// const int Score_dynamic[10] = { 700 , 350 , 200 , 100 , 80 , 60 , 40 , 20 , 10 , 5};

		double inf_value[7][7] = {
			{1,  0,  0,  0,  0,  0,200},
			{0,  0,  0,  0,  0,  0,  0},
			{2,150,  9,  0,  0,  0,  0},
			{2,150, 10, 53,  0,  0,  0},
			{2,150, 10, 55,148,  0,  0},
			{2,150, 10, 55,150,178,  0},
			{0,150, 10, 55,150,180,200},
		};
		int my_num = this->Color ? main_chessboard.Black_Chess_Num : main_chessboard.Red_Chess_Num;
		int opp_num = this->Color ? main_chessboard.Red_Chess_Num : main_chessboard.Black_Chess_Num;
		
		int my_chess = this->Color ? chessboard->black : chessboard->red;
		int opp_chess = this->Color ? chessboard->red : chessboard->black;
		
		int my_pawn = this->Color*7;
		int opp_pawn = (this->Color^1)*7;
		int my_king = this->Color*7+6;
		int opp_king = (this->Color^1*7)+6;
		

		double dist_value = 0;
		// 殘局再加入追擊分數
		if(opp_num < 5 || chessboard->NoEatFlip >= 20){
			for(int i = my_pawn;i < my_pawn+7;i++){
				U32 p = chessboard->piece[i];
				while(p){
					U32 lsb =  p & (-p);
					p ^= lsb;
					int my_chess_pos = BboardtoIndex[BitHash(lsb)];
					for(int j = opp_pawn;j < opp_pawn+7;j++){
						U32 q = chessboard->piece[j];
						while(q){
							U32 lsb2 =  q & (-q);
							q ^= lsb2;
							int opp_chess_pos = BboardtoIndex[BitHash(lsb2)];
							double MD = mah_dist[my_chess_pos][opp_chess_pos];
							MD *= 2;
							if(MD == 2){
								dist_value += inf_value[i%7][j%7] / 2.0;
							}
							else{
								dist_value += inf_value[i%7][j%7] * pow(2, 3 - MD);
							}
						}
					}
				}
			}
		}
		
		// const int Pawn_to_King_dist_score[11] = {0, 150, 100, 70, 20, 10, 0, 0, 0, 0, 0};
		//                              0   1   2   3   4
		// const int Pawn_left_score[6] = {0, 120, 40, 30, 24};
		// TODO : proper dynamicly pawn-king value
		double basic_value = 0;
		for(int i = 0;i < 14;i++){
			int p_cnt = chessboard->Chess_Cnt[i];
			if(p_cnt == 0){ continue;}
			// MY CHESS
			if(i/7 == this->Color){
				basic_value += basic_values[i] * p_cnt * 6;
			}
			// OPP CHESS
			else{
				basic_value -= basic_values[i] * p_cnt * 6;
			}
			
		}

		score = (basic_value+dist_value) / 1943 * (WIN - 0.01);
		score = score - opp_num * 0.01;

		// * 獎勵捉雙
		
		if(opp_num < 5 || chessboard->NoEatFlip >= 20){
			for(int i = my_king;i >= my_king-6;i--){
				U32 p = chessboard->piece[i];
				while(p){
					int cnt = 0;
					U32 lsb = p & (-p);
					p ^= lsb;
					int src = BboardtoIndex[BitHash(lsb)];
					U32 dst = 0;
					if(i % 7 == 0){
						dst = pMoves[src] & (chessboard->piece[opp_pawn] | chessboard->piece[opp_pawn+6]);
					}
					else if(i%7 == 2){
						dst = pMoves[src] & (chessboard->piece[opp_pawn] | chessboard->piece[opp_pawn+1]);
					}
					else if(i%7 == 3){
						dst = pMoves[src] & (chessboard->piece[opp_pawn] | chessboard->piece[opp_pawn+1] | chessboard->piece[opp_pawn+2]);
					}
					else if(i%7 == 4){
						dst = pMoves[src] & (chessboard->piece[opp_pawn] | chessboard->piece[opp_pawn+1] | chessboard->piece[opp_pawn+2] | chessboard->piece[opp_pawn+3]);
					}
					else if(i%7 == 5){
						dst = pMoves[src] & (chessboard->piece[opp_pawn] | chessboard->piece[opp_pawn+1] | chessboard->piece[opp_pawn+2] | chessboard->piece[opp_pawn+3] | chessboard->piece[opp_pawn+4]);
					}
					else if(i%7 == 6){
						dst = pMoves[src] & (chessboard->piece[opp_pawn+1] | chessboard->piece[opp_pawn+2] | chessboard->piece[opp_pawn+3] | chessboard->piece[opp_pawn+4] | chessboard->piece[opp_pawn+5]);
					}

					while(dst){
						U32 dst_lsb = dst & (-dst); // Find LSB
						dst ^= dst_lsb; // Remove LSB
						// score += 0.005;
						cnt++;
					}
					score += (0.005+cnt*0.001)*cnt;
				}
			}

		}
	}
	return score;
}

bool MyAI::isDangerous(const ChessBoard *chessboard, const int ply, const int color){
	int Moves[2048];
	int move_count;
	int my_king = (this->Color)*7 + 6;
	// fprintf(stderr, "1 ");
	if(chessboard->NoEatFlip == 0){
		if(ply/100 == ply%100){
			return false;
		}
		else{
			return true;
		}
	}
	move_count = Expand(chessboard->Board, this->Color^1, Moves, chessboard->piece, chessboard->all_chess, chessboard->occupied, chessboard->red, chessboard->black);
	
	for(int i = 0;i < move_count;i++){
		if(chessboard->Board[Moves[i]%100] == my_king){
			return true;
		}
	}
	return false;
}

// always use my point of view, so use this->Color
double MyAI::OldEvaluate(const ChessBoard* chessboard, const int legal_move_count, const int color, const int depth){
	// score = My Score - Opponent's Score
	// offset = <WIN + BONUS> to let score always not less than zero

	double score = 0;
	bool finish;

	if(legal_move_count == 0){ // Win, Lose
		if(color == this->Color){ // Lose
			score = -1;
		}else{ // Win
			score = 1 - double(depth)*0.01;
		}
		finish = true;
	}else if(isDraw(chessboard)){ // Draw
		// score += DRAW - DRAW;
		score = 0;
		finish = true;
	}else{ // no conclusion
		// static material values
		// cover and empty are both zero
		static const double values[14] = {
			  1,180,  6, 18, 90,270,810,  
			  1,180,  6, 18, 90,270,810
		};
		
		double piece_value = 0;
		for(int i = 0;i < 32;i++){
			if(chessboard->Board[i] != CHESS_EMPTY && 
				chessboard->Board[i] != CHESS_COVER){
				if(chessboard->Board[i] / 7 == this->Color){
					piece_value += values[chessboard->Board[i]];
				}else{
					piece_value -= values[chessboard->Board[i]];
				}
			}
		}
		// linear map to (-<WIN>, <WIN>)
		// score max value = 1*5 + 180*2 + 6*2 + 18*2 + 90*2 + 270*2 + 810*1 = 1943
		// <ORIGINAL_SCORE> / <ORIGINAL_SCORE_MAX_VALUE> * (<WIN> - 0.01)
		piece_value = piece_value / 1943 * (WIN - 0.01);
		score += piece_value;
		finish = false;
	}

	// Bonus (Only Win / Draw)
	// if(finish){
	// 	if((this->Color == RED && chessboard->Red_Chess_Num > chessboard->Black_Chess_Num) ||
	// 		(this->Color == BLACK && chessboard->Red_Chess_Num < chessboard->Black_Chess_Num)){
	// 		if(!(legal_move_count == 0 && color == this->Color)){ // except Lose
	// 			double bonus = BONUS / BONUS_MAX_PIECE * 
	// 				abs(chessboard->Red_Chess_Num - chessboard->Black_Chess_Num);
	// 			if(bonus > BONUS){ bonus = BONUS;} // limit
	// 			score += bonus;
	// 		}
	// 	}else if((this->Color == RED && chessboard->Red_Chess_Num < chessboard->Black_Chess_Num) ||
	// 		(this->Color == BLACK && chessboard->Red_Chess_Num > chessboard->Black_Chess_Num)){
	// 		if(!(legal_move_count == 0 && color != this->Color)){ // except Lose
	// 			double bonus = BONUS / BONUS_MAX_PIECE * 
	// 				abs(chessboard->Red_Chess_Num - chessboard->Black_Chess_Num);
	// 			if(bonus > BONUS){ bonus = BONUS;} // limit
	// 			score -= bonus;
	// 		}
	// 	}
	// }
	// fprintf(stderr, "score = %f\n", score);
	return score;
}

double MyAI::Nega_max(ChessBoard chessboard, int* move, const int color, const int depth, const int remain_depth){

	int Moves[2048], Chess[2048];
	int move_count = 0, flip_count = 0, remain_count = 0, remain_total = 0;
	
	// move
	move_count = Expand(chessboard.Board, color, Moves, chessboard.piece, chessboard.all_chess, chessboard.occupied, chessboard.red, chessboard.black);
	
	// flip
	for(int j = 0;j < 14;j++){ // find remain chess
		if(chessboard.CoverChess[j] > 0){
			Chess[remain_count] = j;
			remain_count++;
			remain_total += chessboard.CoverChess[j];
		}
	}
	for(int i = 0;i < 32;i++){ // find cover
		if(chessboard.Board[i] == CHESS_COVER){
			Moves[move_count + flip_count] = i*100+i;
			flip_count++;
		}
	}
	if(remain_depth <= 0 || // reach limit of depth
		chessboard.Red_Chess_Num == 0 || // terminal node (no chess type)
		chessboard.Black_Chess_Num == 0 || // terminal node (no chess type)
		move_count+flip_count == 0 // terminal node (no move type)
		){
		this->node++;
		// odd: *-1, even: *1
		return Evaluate(&chessboard, move_count+flip_count, color, depth) * (depth&1 ? -1 : 1);
	}
	else{
		double m = -DBL_MAX;
		int new_move;
		// search deeper
		for(int i = 0;i < move_count;i++){ // move
			ChessBoard new_chessboard = chessboard;
			
			MakeMove(&new_chessboard, Moves[i], 0); // 0: dummy
			double t = -Nega_max(new_chessboard, &new_move, color^1, depth+1, remain_depth-1);
			if(t > m){ 
				m = t;
				*move = Moves[i];
			}else if(t == m){
				bool r = rand()%2;
				if(r) *move = Moves[i];
			}
		}
		for(int i = move_count;i < move_count + flip_count;i++){ // flip
			double total = 0;
			for(int k = 0;k < remain_count;k++){
				ChessBoard new_chessboard = chessboard;
				MakeMove(&new_chessboard, Moves[i], Chess[k]);
				double t = -Nega_max(new_chessboard, &new_move, color^1, depth+1, remain_depth-1);
				total += chessboard.CoverChess[Chess[k]] * t;
			}

			double E_score = (total / remain_total); // calculate the expect value of flip
			if(E_score > m){ 
				m = E_score;
				*move = Moves[i];
			}else if(E_score == m){
				bool r = rand()%2;
				if(r) *move = Moves[i];
			}
		}
		
		// Random Flip
		// fprintf(stderr, "Flip :");
		// if(move_count == 0 && flip_count > 0){
		// 	char tmp[6];
		// 	int v1 = rand() % flip_count;
		// 	int best_move = Moves[v1];
		// 	int StartPoint = best_move/100;
		// 	int EndPoint   = best_move%100;
		// 	sprintf(tmp, "%c%c-%c%c",'a'+(StartPoint%4),'1'+(7-StartPoint/4),'a'+(EndPoint%4),'1'+(7-EndPoint/4));
		// 	fprintf(stderr,"%s\n", tmp);
		// 	fflush(stderr);
		// 	(*move) = Moves[v1];
		// }
		return m;
	}
}

double MyAI::Nega_Scout(const ChessBoard chessboard, int* move, const int color, double alpha, double beta, const int depth, const int remain_depth, bool doNotFlip, const int current_depth_limit, bool in_null, bool in_lmr, bool from_flip){
	int Moves[2048];
	int move_count = 0, flip_count = 0, remain_total = 0;
	for(int j = 13;j >= 0;j--){
		remain_total += chessboard.CoverChess[j];
	}
	// WALK
	move_count = Expand(chessboard.Board, color, Moves, chessboard.piece, chessboard.all_chess, chessboard.occupied, chessboard.red, chessboard.black);
	
	// FLIP
	if(!doNotFlip || move_count == 0){
		U32 to_flip = 0;
		U32 opp_pos = (color == 0) ? chessboard.black : chessboard.red;
		U32 p = opp_pos;
		while(p){
			U32 lsb = p & -p;
			p ^= lsb;
			to_flip |= flip_masks[BboardtoIndex[BitHash(lsb)]];
		}
		U32 my_pos = (color == 0) ? chessboard.red : chessboard.black;
		to_flip -= (to_flip & my_pos);
		while(to_flip){
			U32 lsb = to_flip & -to_flip;
			to_flip ^= lsb;
			int src = BboardtoIndex[BitHash(lsb)];
			if(chessboard.Board[src] == CHESS_COVER){
				Moves[move_count + flip_count] = src*100+src;
				flip_count++;
			}
		}
		// If mask no find flip, pick all
		if(flip_count == 0 || flip_count <= 5){
			// opening : pick a5
			if(remain_total > 30){
				Moves[move_count + flip_count] = 13*100+13;
				flip_count++;
			}
			// get first 5 to flip
			else{
				for(int j = 0;j < 32;j++){ // find cover
					int i = flip_order[j];
					if(chessboard.Board[i] == CHESS_COVER){
						Moves[move_count + flip_count] = i*100+i;
						flip_count++;
						if(flip_count > 5) break;
					}
				}
			}
		}
	}

	// * HT table : Sort Moves with Weight in Table
	// if(depth == 0){
	// 	fprintf(stderr, "Original Move Order: \n");
	// 	for(int i = 0;i < move_count+flip_count;i++){ fprintf(stderr, "%04d ", Moves[i]);}
	// 	fprintf(stderr, "\n");
	// }

	// std::sort(Moves, Moves+move_count+flip_count-1, [] (const int lhs, const int rhs) {return HT[lhs] > HT[rhs];});
	
	// if(depth == 0){
	// 	fprintf(stderr, "History Table Reorder: \n");
	// 	for(int i = 0;i < move_count+flip_count;i++){ fprintf(stderr, "%04d ", Moves[i]);}
	// 	fprintf(stderr, "\n");
	// }

	// * REFUTATION TABLE : Reorder Moves
	int _k = 0;
	for(int i = current_depth_limit - 1;i >= 1;i--){
		for(int j = _k;j < move_count+flip_count;j++){
			if(PVs[i][depth] == Moves[j]){
				int tmp = Moves[_k];
				Moves[_k] = Moves[j];
				Moves[j] = tmp;
				_k = _k + 1;
				break;
			}
		}
	}

	// if(depth == 0){
	// 	fprintf(stderr, "Refutation Reorder: \n");
	// 	for(int i = 0;i < move_count+flip_count;i++){ fprintf(stderr, "%04d ", Moves[i]);}
	// 	fprintf(stderr, "\n");
	// }


	// TRANS TABLE
	
	// transposition* trans_tmp = (color == 0) ? & Red_Trans_Table[chessboard.hash%TRANS_TABLE_SIZE] : &Black_Trans_Table[chessboard.hash%TRANS_TABLE_SIZE];
	// if(trans_tmp->taken == true && trans_tmp->position == chessboard.hash){
	// 	hit_cnt ++;
	// 	if(trans_tmp->depth < remain_depth){ }
	// 	else{
	// 		if(trans_tmp->exact == EXACT){
	// 			hit_exact_use++;
	// 			return trans_tmp->value;
	// 		}
	// 		else if(trans_tmp->exact == LOWER){
	// 			alpha = std::max(alpha, trans_tmp->value);
	// 		}
	// 		else if(trans_tmp->exact == UPPER){
	// 			alpha = std::max(alpha, trans_tmp->value);
	// 			// beta = std::min(beta, trans_tmp->value);
	// 		}
	// 	}
	// }
	// else{
	// 	if(trans_tmp-> taken == true){
	// 		hit_write++;
	// 	}
	// 	trans_tmp->taken = false;
	// }
	
	if(	isTimeUp(time_limit)||
		remain_depth <= 0 || // reach limit of depth
		chessboard.Red_Chess_Num == 0 || // terminal node (no chess type)
		chessboard.Black_Chess_Num == 0 || // terminal node (no chess type)
		(move_count+flip_count) == 0 // terminal node (no move type) // b = 0
		){
		this->node++;
		for(int i = depth;i < current_depth_limit;i++){
			PVs[current_depth_limit][i] = -1;
		}
		return Evaluate(&chessboard, move_count+flip_count, color, depth) * (depth&1 ? -1 : 1);
	}
	else{
		double m = -DBL_MAX; // current lower bound;for FAIL SOFT
		double n = beta; // current upper bound
		int new_move;
		// * TRANS TABLE : CHECK HASH HIT
		// if(trans_tmp->taken == false){
		// 	trans_tmp->taken = true;
		// 	trans_tmp->position = chessboard.hash;
		// 	trans_tmp->depth = remain_depth;
		// }

		// int NULL_MOVE_R = 2;

		for(int i = 0;i < move_count + flip_count;i++){
			double t;
			if(Moves[i]/100 == Moves[i]%100){
				// t = Star0(chessboard, &new_move, Moves[i], color, std::max(alpha, -10.0), 10.0, depth+3, remain_depth-3, current_depth_limit, in_null, in_lmr);
				
				t = Star0(chessboard, &new_move, Moves[i], color, -10.0, -std::max(alpha, -10.0), depth+3, remain_depth-3, current_depth_limit, in_null, in_lmr);
				// if(t > 0) t *= 0.8;
				// trans_tmp->value = t;
				if(t > m){
					m = t;
					*move = Moves[i];
					// REFUTATION TABLE : Store Good Moves
					PVs[current_depth_limit][depth] = Moves[i];
				}
				if(m >= beta){
					// trans_tmp->value = m;
					// trans_tmp->exact = LOWER;
					// HT TABLE : counter update
					// HT[*move] += HT_WEIGHT;

					PVs[current_depth_limit][depth] = Moves[i];
					
					// total_b += i;
					// total_round++;
					
					return m;
				}
				
				// if(depth == 0){
				// 	char tmp[6];
				// 	int StartPoint = Moves[i]/100, EndPoint = Moves[i]%100;
				// 	sprintf(tmp, "%c%c-%c%c",'a'+(StartPoint%4),'1'+(7-StartPoint/4),'a'+(EndPoint%4),'1'+(7-EndPoint/4));
				// 	fprintf(stderr, "%d: Flip %s, scout value = %f, m = %f, beta = %f, n = %f\n", i , tmp, t, m, beta, n);
				// 	fflush(stderr);
				// }
			}
			// * MOVE
			else{
				// fprintf(stderr, "depth = %d, In Move %04d \n", depth, Moves[i]);
				ChessBoard new_chessboard = chessboard;
				MakeMove(&new_chessboard, Moves[i], 0);

				// * Null Move Pruning
				// if(depth == 0) fprintf(stderr, "in NULL condition: %d, %d, %d\n", remain_depth <= (2 + 3), in_null, isDangerous(&new_chessboard, Moves[i], color));
				// if(!(remain_depth <= (NULL_MOVE_R + 3) || in_null || isDangerous(&new_chessboard, Moves[i], color))){
				// 	if(depth == 0) fprintf(stderr, "in NULL\n");
				// 	double null_score = Nega_Scout(chessboard, &new_move, color, alpha, alpha+1, depth+NULL_MOVE_R+1, remain_depth-NULL_MOVE_R-1, true, current_depth_limit, true, in_lmr, from_flip);
				// 	if(null_score >= beta){
				// 		if(depth == 0) fprintf(stderr, "return null score\n");
				// 		return null_score;
				// 	}
				// }

				int new_remain_depth = remain_depth;
				
				// * LMR
				bool LMR_flag = false;
				// TODO : some bug here
				// if(in_lmr || i <= LMR_K || remain_depth <= LMR_H+3 || dangerous){
				// 	if(depth == 0) fprintf(stderr, "no LMR\n");
				// 	new_remain_depth = remain_depth;
				// 	LMR_flag = in_lmr;
				// }
				// else{
				// 	if(depth == 0) fprintf(stderr, "in LMR, depth reduced!\n");
				// 	new_remain_depth = remain_depth - LMR_H;
				// 	LMR_flag = true;
				// }
				t = -Nega_Scout(new_chessboard, &new_move, color^1, -n, -std::max(alpha, m), depth+1, remain_depth-1, false, current_depth_limit, in_null, false, from_flip);
				
				// * DYNAMIC DEPTH EXTENSION
				if(	t > m &&
					(!from_flip) &&
					isDangerous(&new_chessboard, Moves[i], color) && 
					depth < current_depth_limit+1 && 
					depth >= current_depth_limit-1 && 
					color == this->Color
					){
					// fprintf(stderr, "depth = %d\n", depth);
					new_remain_depth = remain_depth + 1;
					// fprintf(stderr, "before extend: %f\n", t);
					t = -Nega_Scout(new_chessboard, &new_move, color^1, -n, -std::max(alpha, m), depth+1, new_remain_depth-1, false, current_depth_limit, in_null, LMR_flag, from_flip);
					// fprintf(stderr, "after extend: %f\n", t);
				}

				if(isDraw(&new_chessboard)){ t = t - 800 * (depth&1 ? -1 : 1);}

				// trans_tmp->value = t;
				if(t > m){
					if(n == beta || remain_depth < 3 || t >= beta){
						m = t;
					}
					else{
						m = -Nega_Scout(new_chessboard, &new_move, color^1, -beta, -t, depth+1, new_remain_depth-1, false, current_depth_limit, in_null, in_lmr, from_flip);
					}
					*move = Moves[i];
					// REFUTATION TABLE : Store Good Moves
					PVs[current_depth_limit][depth] = Moves[i];
				}
				if(m >= beta){
					// trans_tmp->value = m;
					// trans_tmp->exact = LOWER;
					// HT TABLE : counter update
					// HT[*move] += HT_WEIGHT;
					PVs[current_depth_limit][depth] = Moves[i];
					// total_b += i;
					// total_round++;
					return m;
				}
				// if(depth == 0){
				// 	char tmp[6];
				// 	int StartPoint = Moves[i]/100, EndPoint = Moves[i]%100;
				// 	sprintf(tmp, "%c%c-%c%c",'a'+(StartPoint%4),'1'+(7-StartPoint/4),'a'+(EndPoint%4),'1'+(7-EndPoint/4));
				// 	fprintf(stderr, "%d: Move %s, scout value = %f, m = %f, beta = %f, n = %f\n", i , tmp, t, m, beta, n);
				// 	fflush(stderr);
				// }
				
			}
			n = std::max(alpha, m)+1;
		}

		PVs[current_depth_limit][depth] = Moves[move_count+flip_count-1];

		// total_b += move_count+flip_count-1;
		// total_round++;
		// * Transposition Table
		
		// if(m > alpha){
		// 	trans_tmp->value = m;
		// 	trans_tmp->exact = EXACT;
		// }
		// else{
		// 	trans_tmp->value = m;
		// 	trans_tmp->exact = UPPER;
		// }
		
		// * HT heuristic
		
		// HT TABLE : counter update
		// HT[*move] += HT_WEIGHT;
		

		return m;
	}
}

double MyAI::Star0(const ChessBoard chessboard, int* move, int move_i, const int color, double alpha, double beta, const int depth, const int remain_depth, const int current_depth_limit, bool in_null, bool in_lmr){
	bool debug = false;

	int Chess[14];
	int remain_count = 0, remain_total = 0;
	int order[7] = {6, 0, 1, 5, 4, 3, 2};
	// find remain chess
	for(int i = 0;i < 7;i++){
		int idx = color * 7 + order[i];
		if(chessboard.CoverChess[idx] > 0){
			Chess[remain_count] = idx;
			remain_count++;
			remain_total += chessboard.CoverChess[idx];
		}
	}
	for(int i = 0;i < 7;i++){
		int idx = (color^1) * 7 + order[i];
		if(chessboard.CoverChess[idx] > 0){
			Chess[remain_count] = idx;
			remain_count++;
			remain_total += chessboard.CoverChess[idx];
		}
	}
	double v_exp = 0;
	
	for(int i = 0;i < remain_count;i++){
		ChessBoard new_chessboard = chessboard;
		int new_move;
		MakeMove(&new_chessboard, move_i, Chess[i]);
		double t = -Nega_Scout(new_chessboard, &new_move, color^1, alpha, beta, depth, remain_depth, true, current_depth_limit, in_null, in_lmr, true);

		v_exp += t * double(chessboard.CoverChess[Chess[i]])/double(remain_total);
		

		if(depth == 1 && debug){ fprintf(stderr, " star 0 i = %d, chess = %d, v_exp = %f\n", i, Chess[i], v_exp);}
	}
	return v_exp;
}

double MyAI::Star1(const ChessBoard chessboard, int* move, int move_i, const int color, double alpha, double beta, const int depth, const int remain_depth, const int current_depth_limit, bool in_null, bool in_lmr){
	bool debug = false;
	double Pr[14];
	int Chess[14];
	int remain_count = 0, remain_total = 0;
	int order[7] = {6, 5, 0, 1, 4, 3, 2};
	if(chessboard.Chess_Cnt[(this->Color^1)*7+6] == 0){
		order[0] = 6;
		order[1] = 5;
		order[2] = 4;
		order[3] = 3;
		order[4] = 2;
		order[5] = 1;
		order[6] = 0;
	}
	// find remain chess
	for(int i = 0;i < 7;i++){
		int idx = color * 7 + order[i];
		if(chessboard.CoverChess[idx] > 0){
			Chess[remain_count] = idx;
			remain_count++;
			remain_total += chessboard.CoverChess[idx];
		}
	}
	for(int i = 0;i < 7;i++){
		int idx = (color^1) * 7 + order[i];
		if(chessboard.CoverChess[idx] > 0){
			Chess[remain_count] = idx;
			remain_count++;
			remain_total += chessboard.CoverChess[idx];
		}
	}

	// count each dark chess probability
	for(int i = 0;i < remain_count;i++){
		int ch = Chess[i];
		Pr[i] = double(chessboard.CoverChess[ch]) / double(remain_total);
	}
	double v_max = 100, v_min = -100;
	double A = (alpha - v_max) / Pr[0] + v_max, B = (beta - v_min) / Pr[0] + v_min;
	double star_m = v_min, star_M = v_max, v_exp = 0;
	
	for(int i = 0;i < remain_count;i++){
		ChessBoard new_chessboard = chessboard;
		int new_move;
		MakeMove(&new_chessboard, move_i, Chess[i]);
		double l_bound = std::max(A, v_min), u_bound = std::min(B, v_max);
		double t = -Nega_Scout(new_chessboard, &new_move, color^1, -u_bound, -l_bound, depth, remain_depth, true, current_depth_limit, in_null, in_lmr, true);
		v_exp += Pr[i] * t;
		if(depth == 1 && debug){ fprintf(stderr, "i = %d, t = %f, star_m = %f, star_M = %f, v_exp = %f\n", i, t, star_m, star_M, v_exp);}
		// cut off I
		if(t >= B){
			if(depth == 1 && debug){ fprintf(stderr, "star_m = %f, v_exp = %f, return star_m \n", star_m, v_exp);}
			return star_m;
		}
		// cut off II
		if(t <= A){
			if(depth == 1 && debug){ fprintf(stderr, "star_M = %f, v_exp = %f, return star_M \n", star_M, v_exp);}
			return star_M;
		}
		if(i < remain_count-1){
			A = (Pr[i] * A + Pr[i+1] * v_max - Pr[i] * t) / Pr[i+1];
			B = (Pr[i] * B + Pr[i+1] * v_min - Pr[i] * t) / Pr[i+1];
		}
		star_m += Pr[i] * (t - v_min);
		star_M += Pr[i] * (t - v_max);
	}
	return v_exp;
}

bool MyAI::myRepetitionDraw(const ChessBoard* chessboard){
	// No Eat Flip
	if(chessboard->NoEatFlip >= NOEATFLIP_LIMIT-15){
		return true;
	}
	
	// Position Repetition
	int last_idx = chessboard->HistoryCount - 1;
	int idx = last_idx - 2;
	int smallest_repetition_idx = last_idx - (chessboard->NoEatFlip / (POSITION_REPETITION_LIMIT-1));
	// check loop
	while(idx >= 0 && idx >= smallest_repetition_idx){
		if(chessboard->History[idx] == chessboard->History[last_idx]){
			int repetition_size = last_idx - idx;
			bool isEqual = true;
			for(int i = 1;i < (POSITION_REPETITION_LIMIT-1) && isEqual;++i){
				for(int j = 0;j < repetition_size;++j){
					int src_idx = last_idx - j;
					int checked_idx = last_idx - i*repetition_size - j;
					if(chessboard->History[src_idx] != chessboard->History[checked_idx]){
						isEqual = false;
						break;
					}
				}
			}
			if(isEqual){
				return true;
			}
		}
		idx -= 2;
	}
	return false;
}

bool MyAI::isDraw(const ChessBoard* chessboard){
	// No Eat Flip
	if(chessboard->NoEatFlip >= NOEATFLIP_LIMIT){
		return true;
	}

	// Position Repetition
	int last_idx = chessboard->HistoryCount - 1;
	// -2: my previous ply
	int idx = last_idx - 2;
	// All ply must be move type
	int smallest_repetition_idx = last_idx - (chessboard->NoEatFlip / POSITION_REPETITION_LIMIT);
	// check loop
	while(idx >= 0 && idx >= smallest_repetition_idx){
		if(chessboard->History[idx] == chessboard->History[last_idx]){
			// how much ply compose one repetition
			int repetition_size = last_idx - idx;
			bool isEqual = true;
			for(int i = 1;i < POSITION_REPETITION_LIMIT && isEqual;++i){
				for(int j = 0;j < repetition_size;++j){
					int src_idx = last_idx - j;
					int checked_idx = last_idx - i*repetition_size - j;
					if(chessboard->History[src_idx] != chessboard->History[checked_idx]){
						isEqual = false;
						break;
					}
				}
			}
			if(isEqual){
				return true;
			}
		}
		idx -= 2;
	}

	return false;
}

bool MyAI::isFinish(const ChessBoard* chessboard, int move_count){
	return (
		chessboard->Red_Chess_Num == 0 || // terminal node (no chess type)
		chessboard->Black_Chess_Num == 0 || // terminal node (no chess type)
		move_count == 0 || // terminal node (no move type)
		isDraw(chessboard) // draw
	);
}

// Display chess board
void MyAI::Print_Chessboard()
{	
	char Mes[1024] = "";
	char temp[1024];
	char myColor[10] = "";
	if(Color == -99)
		strcpy(myColor, "Unknown");
	else if(this->Color == RED)
		strcpy(myColor, "Red");
	else
		strcpy(myColor, "Black");
	sprintf(temp, "------------%s-------------\n", myColor);
	strcat(Mes, temp);
	strcat(Mes, "<8> ");
	for(int i = 0;i < 32;i++){
		if(i != 0 && i % 4 == 0){
			sprintf(temp, "\n<%d> ", 8-(i/4));
			strcat(Mes, temp);
		}
		char chess_name[4] = "";
		Print_Chess(this->main_chessboard.Board[i], chess_name);
		sprintf(temp,"%5s", chess_name);
		strcat(Mes, temp);
	}
	strcat(Mes, "\n\n     ");
	for(int i = 0;i < 4;i++){
		sprintf(temp, " <%c> ", 'a'+i);
		strcat(Mes, temp);
	}
	strcat(Mes, "\n\n");
	printf("%s", Mes);
}

// Print chess
void MyAI::Print_Chess(int chess_no,char *Result){
	// XX -> Empty
	if(chess_no == CHESS_EMPTY){	
		strcat(Result, " - ");
		return;
	}
	// OO -> DarkChess
	else if(chess_no == CHESS_COVER){
		strcat(Result, " X ");
		return;
	}

	switch(chess_no){
		case 0:
			strcat(Result, " P ");
			break;
		case 1:
			strcat(Result, " C ");
			break;
		case 2:
			strcat(Result, " N ");
			break;
		case 3:
			strcat(Result, " R ");
			break;
		case 4:
			strcat(Result, " M ");
			break;
		case 5:
			strcat(Result, " G ");
			break;
		case 6:
			strcat(Result, " K ");
			break;
		case 7:
			strcat(Result, " p ");
			break;
		case 8:
			strcat(Result, " c ");
			break;
		case 9:
			strcat(Result, " n ");
			break;
		case 10:
			strcat(Result, " r ");
			break;
		case 11:
			strcat(Result, " m ");
			break;
		case 12:
			strcat(Result, " g ");
			break;
		case 13:
			strcat(Result, " k ");
			break;
	}
}

