#include "position.hpp"

namespace chess
{
	void position::print(const color_t color_to_move) const
	{
		std::cout << (color_to_move == white ? "White" : "Black") << " to move:\n";
		print();
	}
	void position::print() const
	{
		for (size_t i = 0; i < _position.size(); ++i)
		{
			if (i % 8 == 0) std::cout << ' ';

			const piece piece = piece_at(i);

			if (piece.is_empty()) std::cout << ".";
			else if (piece.is_white())
			{
				if (piece.is_pawn()) std::cout << "P";
				else if (piece.is_rook()) std::cout << "R";
				else if (piece.is_bishop()) std::cout << "B";
				else if (piece.is_knight()) std::cout << "N";
				else if (piece.is_queen()) std::cout << "Q";
				else if (piece.is_king()) std::cout << "K";
			}
			else if (piece.is_black())
			{
				if (piece.is_pawn()) std::cout << "p";
				else if (piece.is_rook()) std::cout << "r";
				else if (piece.is_bishop()) std::cout << "b";
				else if (piece.is_knight()) std::cout << "n";
				else if (piece.is_queen()) std::cout << "q";
				else if (piece.is_king()) std::cout << "k";
			}

			if (i % 8 == 7) std::cout << '\n';
		}
	}

	std::array<position, max_ply * max_n_of_moves> positions{};
}
