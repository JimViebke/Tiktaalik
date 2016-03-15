/*
Jim Viebke
June 15 2014
*/

#include <iostream> // console input and output
#include <string> // for strings
#include <vector> // used to represent the board

#include "board.h"

void print_commands()
{
	std::cout << "\n\n\tEnter a command:\n"
		<< "\n\tload new game\t\tengine white"
		<< "\n\tload FEN     \t\tengine black"
		<< "\n\tload PGN     \t\tevaluate"
		<< "\n\n> ";
}

void menu()
{
	// set up variables for input
	std::string user_input;
	bool quit = false;

	while (!quit)
	{
		print_commands(); // print commands to the console

		std::getline(std::cin, user_input); // read in the user's command

		if (user_input == "load new game")
		{
			std::cout << std::endl;
		}
		else if (user_input == "load FEN")
		{
			// board.position = load_FEN();
			std::cout << "\n\tEnter FEN: ";
		}
		else if (user_input == "evaluate")
		{
			std::cout << std::endl;

			std::cout << "Board value is [evaluation], where 1 point = 1 centipawn.\n";
			std::cout << "A positive value is in white's favor, a negative is black's.\n";
		}
		else if (user_input == "quit")
		{
			quit = true;
		} // end main menu structure

	} // end while
}

void print_board(const Board & board)
{
	const std::vector<std::vector<Piece>> current_board = board.get_board();

	for (const std::vector<Piece> & row : current_board)
	{
		for (const Piece & piece : row)
		{
			if (piece.is_empty()) std::cout << ".";
			else if (piece.is_white())
			{
				if (piece.is_king()) std::cout << "K";
			}
			else if (piece.is_black())
			{
				if (piece.is_king()) std::cout << "k";
			}
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

int main()
{
	Board start_board(layouts::test_board);

	std::list<Board> ply1 = start_board.get_child_boards();
	std::list<Board> ply2;

	for (auto it = ply1.begin(); it != ply1.end(); ++it)
	{
		std::list<Board> temp = it->get_child_boards();
		ply2.splice(ply2.end(), temp);
	}

	for (const Board & board : ply2)
	{
		print_board(board);
	}

	std::cout << ply2.size() << std::endl;
}
