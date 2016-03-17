/*
Jim Viebke
June 15 2014
*/

#include <iostream> // console input and output
#include <string> // for strings
#include <vector> // used to represent the board
#include <map> // for node depth counting

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

inline void offset(const unsigned & offset)
{
	for (unsigned i = 0; i < offset; ++i) { std::cout << ' '; }
}

void print_board(const Board & board, const unsigned & offset = 0)
{
	for (unsigned rank = 0; rank < 8; ++rank)
	{
		// add indent
		for (unsigned i = 0; i < offset * 9; ++i) std::cout << ' ';

		for (unsigned file = 0; file < 8; ++file)
		{
			const Piece & piece = board.piece_at(rank, file);

			if (piece.is_empty()) std::cout << ".";
			else if (piece.is_white())
			{
				if (piece.is_bishop()) std::cout << "B";
				else if (piece.is_rook()) std::cout << "R";
				else if (piece.is_queen()) std::cout << "Q";
				else if (piece.is_king()) std::cout << "K";
			}
			else if (piece.is_black())
			{
				if (piece.is_bishop()) std::cout << "b";
				else if (piece.is_rook()) std::cout << "r";
				else if (piece.is_queen()) std::cout << "q";
				else if (piece.is_king()) std::cout << "k";
			}
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

void print_board(const std::list<Board> & boards)
{
	for (unsigned rank = 0; rank < 8; ++rank)
	{
		// render this rank for each board
		for (std::list<Board>::const_iterator it = boards.cbegin(); it != boards.cend(); ++it)
		{
			for (unsigned file = 0; file < 8; ++file)
			{
				const Piece & piece = it->piece_at(rank, file);

				if (piece.is_empty()) std::cout << ".";
				else if (piece.is_white())
				{
					if (piece.is_bishop()) std::cout << "B";
					else if (piece.is_rook()) std::cout << "R";
					else if (piece.is_queen()) std::cout << "Q";
					else if (piece.is_king()) std::cout << "K";
				}
				else if (piece.is_black())
				{
					if (piece.is_bishop()) std::cout << "b";
					else if (piece.is_rook()) std::cout << "r";
					else if (piece.is_queen()) std::cout << "q";
					else if (piece.is_king()) std::cout << "k";
				}
			}


			// add indent if there is another board to print
			if (it != --boards.cend())
				for (unsigned i = 0; i < 5; ++i) std::cout << ' ';
		}

		std::cout << std::endl;
	}
	std::cout << std::endl;
}

class Node
{
private:
	Board board;
	unsigned my_ply_depth;
	std::list<Node> child_nodes;

	Node(const Board & set_board, unsigned set_my_ply_depth) :
		board(set_board), my_ply_depth(set_my_ply_depth) {}

public:
	Node(const Board & set_board) :
		board(set_board), my_ply_depth(0) {}

	void generate_ply(const unsigned & depth)
	{
		// if we have not reached the ply depth
		if (my_ply_depth < depth)
		{
			// generate child boards for this position
			const std::list<Board> temp = board.get_child_boards();

			// for each child board, add a node
			for (const Board & board : temp)
			{
				child_nodes.push_back(Node(board, my_ply_depth + 1));

				// make a recursive call to continue populating down the tree
				child_nodes.back().generate_ply(depth);
			}
		}
	}

	void print_all() const
	{
		static std::list<Board> boards;
		boards.push_back(board);

		print_board(boards);
		std::cin.ignore();

		for (const Node & node : child_nodes)
		{
			node.print_all();
		}
		boards.pop_back();
	}

	void print_size() const
	{
		std::map<size_t, size_t> node_counter;
		size(node_counter);

		for (auto it = node_counter.begin(); it != node_counter.end(); ++it)
			std::cout << "Number of child nodes from ply " << it->first << ": " << it->second << ".\n";
	}

private:
	void size(std::map<size_t, size_t> & node_counter) const
	{
		node_counter[my_ply_depth] += child_nodes.size();
		for (const Node & child : child_nodes)
		{
			child.size(node_counter);
		}
	}
};

int main()
{
	Node parent_position(layouts::test_board);

	parent_position.generate_ply(5);
	parent_position.print_size();
	parent_position.print_all();
}
