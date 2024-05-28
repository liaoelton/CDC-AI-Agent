#ifndef MYAI_INCLUDED
#define MYAI_INCLUDED 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>


typedef unsigned int U32;

#define RED 0
#define BLACK 1
#define CHESS_COVER -1
#define CHESS_EMPTY -2

#define HASH_CHESS_COVER 14
#define HASH_CHESS_EMPTY 15

#define COMMAND_NUM 19

struct ChessBoard{
	int Board[32];
	int CoverChess[14];
	int Red_Chess_Num, Black_Chess_Num;
	int NoEatFlip;
	int History[4096];
	int HistoryCount;
	U32 piece[14] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int Open_Cnt[14] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int Dark_Cnt[14] = {5, 2, 2, 2, 2, 2, 1, 5, 2, 2, 2, 2, 2, 1};
	int Chess_Cnt[14] = {5, 2, 2, 2, 2, 2, 1, 5, 2, 2, 2, 2, 2, 1};
	int Enemy_Cnt[14] = {9, 10, 8, 6, 4, 2, 1, 9, 10, 8, 6, 4, 2, 1};

	U32 red  = 0;
	U32 black = 0;
	U32 occupied = 0; // open
	U32 all_chess = 0xFFFFFFFF; // dark + open
	unsigned long long hash;

	ChessBoard(){}
	ChessBoard(const ChessBoard &chessBoard){
		memcpy(this->Board, chessBoard.Board, 32*sizeof(int));
		memcpy(this->CoverChess, chessBoard.CoverChess, 14*sizeof(int));
		this->Red_Chess_Num = chessBoard.Red_Chess_Num;
		this->Black_Chess_Num = chessBoard.Black_Chess_Num;
		this->NoEatFlip = chessBoard.NoEatFlip;
		memcpy(this->History, chessBoard.History, chessBoard.HistoryCount*sizeof(int));
		this->HistoryCount = chessBoard.HistoryCount;
		
		memcpy(this->piece, chessBoard.piece, 14*sizeof(U32));
		memcpy(this->Open_Cnt, chessBoard.Open_Cnt, 14*sizeof(int));
		memcpy(this->Dark_Cnt, chessBoard.Dark_Cnt, 14*sizeof(int));
		memcpy(this->Chess_Cnt, chessBoard.Chess_Cnt, 14*sizeof(int));
		memcpy(this->Enemy_Cnt, chessBoard.Enemy_Cnt, 14*sizeof(int));
		this->red = chessBoard.red;
		this->black = chessBoard.black;
		this->occupied = chessBoard.occupied; // open
		this->all_chess = chessBoard.all_chess; // dark + open
		this->hash = chessBoard.hash;
	}
};

class MyAI  
{
	const char* commands_name[COMMAND_NUM] = {
		"protocol_version",
		"name",
		"version",
		"known_command",
		"list_commands",
		"quit",
		"boardsize",
		"reset_board",
		"num_repetition",
		"num_moves_to_draw",
		"move",
		"flip",
		"genmove",
		"game_over",
		"ready",
		"time_settings",
		"time_left",
		"showboard",
		"init_board"
	};
public:
	MyAI(void);
	~MyAI(void);

	// commands
	bool protocol_version(const char* data[], char* response);// 0
	bool name(const char* data[], char* response);// 1
	bool version(const char* data[], char* response);// 2
	bool known_command(const char* data[], char* response);// 3
	bool list_commands(const char* data[], char* response);// 4
	bool quit(const char* data[], char* response);// 5
	bool boardsize(const char* data[], char* response);// 6
	bool reset_board(const char* data[], char* response);// 7
	bool num_repetition(const char* data[], char* response);// 8
	bool num_moves_to_draw(const char* data[], char* response);// 9
	bool move(const char* data[], char* response);// 10
	bool flip(const char* data[], char* response);// 11
	bool genmove(const char* data[], char* response);// 12
	bool game_over(const char* data[], char* response);// 13
	bool ready(const char* data[], char* response);// 14
	bool time_settings(const char* data[], char* response);// 15
	bool time_left(const char* data[], char* response);// 16
	bool showboard(const char* data[], char* response);// 17
	bool init_board(const char* data[], char* response);// 18

	// int HT[2048]={0};

private:
	int Color;
	int Red_Time, Black_Time;
	ChessBoard main_chessboard;

#ifdef WINDOWS
	clock_t begin;
#else
	struct timeval begin;
#endif

	// statistics
	int node;

	// Utils
	int GetFin(char c);
	int ConvertChessNo(int input);

	bool isTimeUp(double time_limit);


	// Board
	void initBoardState();
	void initBoardState(const char* data[]);
	void generateMove(char move[6]);
	void MakeMove(ChessBoard* chessboard, const int move, const int chess);
	void MakeMove(ChessBoard* chessboard, const char move[6]);
	bool Referee(const int* board, const int Startoint, const int EndPoint, const int color);
	int Expand(const int* board, const int color,int *Result, const U32 *piece, const U32 all_chess, const U32 occupied, const U32 red, const U32 black);
	double Evaluate(const ChessBoard* chessboard, const int legal_move_count, const int color, const int depth);
	double OldEvaluate(const ChessBoard* chessboard, const int legal_move_count, const int color, const int depth);
	double Nega_max(ChessBoard chessboard, int* move, const int color, const int depth, const int remain_depth);
	bool isDraw(const ChessBoard* chessboard);
	bool isFinish(const ChessBoard* chessboard, int move_count);
	double Star1(const ChessBoard chessboard, int* move, int move_i, const int color, double alpha, double beta, const int depth, const int remain_depth, const int current_depth_limit, bool in_null, bool in_lmr);
	double Star0(const ChessBoard chessboard, int* move, int move_i, const int color, double alpha, double beta, const int depth, const int remain_depth, const int current_depth_limit, bool in_null, bool in_lmr);

	double Nega_Scout(const ChessBoard chessboard, int* move, const int color, const double alpha, const double beta, const int depth, const int remain_depth, bool doNotFlip, const int current_depth_limit, bool in_null, bool in_lmr, bool from_flip);
	bool isDangerous(const ChessBoard *ChessBoard, const int move, const int color);
	bool myRepetitionDraw(const ChessBoard* chessboard);
	// Display
	void Print_Chess(int chess_no,char *Result);
	void Print_Chessboard();
	
};

#endif

