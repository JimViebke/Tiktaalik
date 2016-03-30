/*
Jim Viebke
June 15 2014
*/

#include <iostream> // console input and output
#include <string> // for strings
#include <vector> // used to represent the board
#include <map> // for node depth counting

#include "board.h"
#include "node.h"

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

int main()
{
	Node parent_position(layouts::test_board);

	parent_position.generate_ply(2);
	parent_position.print_size();
	parent_position.print_all();
}
