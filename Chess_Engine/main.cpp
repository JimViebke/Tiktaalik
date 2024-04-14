/*
Jim Viebke
June 15 2014
*/

#include "game.hpp"
#include "node.hpp"

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

			std::cout << "Board value is [evaluation], where 1 point = 1 pawn.\n";
		}
		else if (user_input == "quit")
		{
			quit = true;
		} // end main menu structure

	} // end while
}

int main()
{
	chess::Game game;
	game.run();
}
