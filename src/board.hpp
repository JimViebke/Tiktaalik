#pragma once

#include <array>
#include <variant>
#include <vector>

#include "capture.hpp"
#include "evaluation.hpp"
#include "position.hpp"
#include "types.hpp"

namespace chess
{
	template<color_t color_to_move>
	class board : public colorable<color_to_move>
	{
	private:
		position _position;

		rank _start_rank = -1; // the move that resulted in this position
		file _start_file = -1;
		rank _end_rank = -1;
		file _end_file = -1;

		file en_passant_file = -1;
		bool white_can_castle_ks = true;
		bool white_can_castle_qs = true;
		bool black_can_castle_ks = true;
		bool black_can_castle_qs = true;

		int8_t fifty_move_counter = 0; // counts to 100 ply
		result _result = result::unknown;

		using other_board_t = board<other(color_to_move)>;

	public:
		using child_boards = std::vector<other_board_t>;

		explicit board(const position& set_position,
					   const bool w_castle_ks, const bool w_castle_qs, const bool b_castle_ks, const bool b_castle_qs,
					   const file set_en_passant_file, const int8_t set_fifty_move_counter) : _position(set_position),
			en_passant_file(set_en_passant_file),
			white_can_castle_ks(w_castle_ks),
			white_can_castle_qs(w_castle_qs),
			black_can_castle_ks(b_castle_ks),
			black_can_castle_qs(b_castle_qs),
			fifty_move_counter(set_fifty_move_counter)
		{
		}

		explicit board(const other_board_t& parent_board, const rank start_rank, const file start_file, const rank end_rank, const file end_file)
		{
			// copy the position
			_position = parent_board._position;

			// record the move that resulted in this position
			_start_rank = start_rank;
			_start_file = start_file;
			_end_rank = end_rank;
			_end_file = end_file;

			// check en passant rights
			en_passant_file = (parent_board.piece_at(start_rank, start_file).is_pawn() && std::abs(rank{ start_rank - end_rank }.value()) == 2)
				? start_file : -1;
			// copy castling rights
			white_can_castle_ks = parent_board.white_can_castle_ks;
			white_can_castle_qs = parent_board.white_can_castle_qs;
			black_can_castle_ks = parent_board.black_can_castle_ks;
			black_can_castle_qs = parent_board.black_can_castle_qs;

			// update counter for 50 move rule - increment for a non-pawn move or non-capture move, reset otherwise
			if (!piece_at(start_rank, start_file).is_pawn() || piece_at(end_rank, end_file).is_empty()) ++fifty_move_counter; else fifty_move_counter = 0;

			// check for en passant captures
			if (piece_at(start_rank, start_file).is_pawn() &&
				abs(rank{ start_rank - end_rank }.value()) == 1 && start_file != end_file &&
				piece_at(end_rank, end_file).is_empty())
			{
				fifty_move_counter = 0;
				piece_at(start_rank, end_file) = piece(empty); // the captured pawn will always be on the same rank that the pawn started, and at the same file that the pawn ended
			}
			// if a king is moving
			else if (piece_at(start_rank, start_file).is_king())
			{
				// it can no longer castle either way
				if (parent_board.piece_at(start_rank, start_file).is_white())
					white_can_castle_ks = white_can_castle_qs = false;
				else
					black_can_castle_ks = black_can_castle_qs = false;

				if (abs(file{ start_file - end_file }.value()) > 1) // if the king is castling, move the rook
				{
					if (start_file < end_file) // kingside castle
						make_move(start_rank, 7, start_rank, 5); // move the rook
					else if (start_file > end_file) // queenside castle
						make_move(start_rank, 0, start_rank, 3); // move the rook
				}
			}

			// if a rook moves, it cannot be used to castle
			if (start_rank == 0) // black rooks
			{
				if (start_file == 0)
					black_can_castle_qs = false;
				else if (start_file == 7)
					black_can_castle_ks = false;
			}
			else if (start_rank == 7) // white rooks
			{
				if (start_file == 0)
					white_can_castle_qs = false;
				else if (start_file == 7)
					white_can_castle_ks = false;
			}

			// if a rook is captured, it cannot be used to castle
			if (end_rank == 0) // black rooks
			{
				if (end_file == 0)
					black_can_castle_qs = false;
				else if (end_file == 7) 
					black_can_castle_ks = false;
			}
			else if (end_rank == 7) // white rooks
			{
				if (end_file == 0)
					white_can_castle_qs = false;
				else if (end_file == 7)
					white_can_castle_ks = false;
			}

			// move the piece
			make_move(start_rank, start_file, end_rank, end_file);
		}
		explicit board(const other_board_t& parent_board, const rank start_rank, const file start_file, const rank end_rank, const file end_file,
					   const piece& promote_to) // call this constructor for each pawn promotion
			: board(parent_board, start_rank, start_file, end_rank, end_file)
		{
			piece_at(end_rank, end_file) = piece(promote_to);
		}

		void set_white_can_castle_ks(bool flag) { white_can_castle_ks = flag; }
		void set_white_can_castle_qs(bool flag) { white_can_castle_qs = flag; }
		void set_black_can_castle_ks(bool flag) { black_can_castle_ks = flag; }
		void set_black_can_castle_qs(bool flag) { black_can_castle_qs = flag; }

		result get_result() const { return _result; }

		const std::string move_to_string() const
		{
			std::string result;
			result += _start_file.value() + 'a';
			result += (_start_rank.value() * -1) + 8 + '0';
			result += _end_file.value() + 'a';
			result += (_end_rank.value() * -1) + 8 + '0';
			return result;
		}

		const piece& piece_at(const rank rank, const file file) const
		{
			return _position[to_index(rank, file)];
		}
		piece& piece_at(const rank rank, const file file)
		{
			return _position[to_index(rank, file)];
		}

		bool is_empty(const auto rank, const auto file) const
		{
			return piece_at(rank, file).is_empty();
		}
		bool is_occupied(const rank rank, const file file) const
		{
			return !is_empty(rank, file);
		}

		eval_t evaluate_position() const
		{
			eval_t eval = 0;

			for (size_t i = 0; i < _position.size(); ++i)
			{
				eval += eval::eval(_position[i]); // evaluate material
				eval += eval::piece_square_eval(_position[i], i); // evaluate position
			}

			return eval;
		}

		position get_position() const { return _position; }

		bool is_king_in_check(const color_t check_color) const
		{
			static_assert(sizeof(piece) == 1);

			// load the 64 bytes of the board into two ymm registers
			uint256_t ymm0 = _mm256_loadu_si256((uint256_t*)_position.data() + 0);
			uint256_t ymm1 = _mm256_loadu_si256((uint256_t*)(_position.data() + 32));

			// broadcast the target piece (king | color) to all positions of a vector
			const uint256_t target_mask = _mm256_set1_epi8(detail::king | check_color);

			// find the matching byte
			ymm0 = _mm256_cmpeq_epi8(ymm0, target_mask);
			ymm1 = _mm256_cmpeq_epi8(ymm1, target_mask);
			// extract 2x 32-bit bitmasks
			const uint64_t mask_low = _mm256_movemask_epi8(ymm0);
			const uint64_t mask_high = _mm256_movemask_epi8(ymm1);
			// merge 2x 32-bit bitmasks to 1x 64-bit bitmask
			const uint64_t mask = (mask_high << 32) | mask_low;

			// Scan for the position of the first set bit in the mask.
			// tzcnt returns the width of the type if a bit is not found.
			size_t index = _tzcnt_u64(mask);

			if (index < 64)
			{
				return is_king_in_check(_position[index], index / 8, index % 8);
			}

			return false;
		}

		child_boards& generate_child_boards()
		{
			static child_boards child_boards;

			child_boards.clear();

			for (rank rank = 0; rank < 8; ++rank)
			{
				for (file file = 0; file < 8; ++file)
				{
					// if this piece can move
					if (piece_at(rank, file).is_occupied() && piece_at(rank, file).is_color(color_to_move))
					{
						if (piece_at(rank, file).is_pawn())
						{
							find_pawn_moves(child_boards, rank, file);
						}
						else if (piece_at(rank, file).is_rook())
						{
							find_rook_moves(child_boards, rank, file);
						}
						else if (piece_at(rank, file).is_bishop())
						{
							find_bishop_moves(child_boards, rank, file);
						}
						else if (piece_at(rank, file).is_knight())
						{
							find_knight_moves(child_boards, rank, file);
						}
						else if (piece_at(rank, file).is_queen())
						{
							find_queen_moves(child_boards, rank, file);
						}
						else if (piece_at(rank, file).is_king())
						{
							find_king_moves(child_boards, rank, file);
						}
					} // end if piece can move
				}
			}

			remove_invalid_boards(child_boards);

			if (child_boards.size() == 0)
			{
				if (is_king_in_check(color_to_move))
					_result = (color_to_move == white) ? result::black_wins_by_checkmate : result::white_wins_by_checkmate;
				else
					_result = result::draw_by_stalemate;
			}

			return child_boards;
		}

		bool is_valid_position() const
		{
			// It is fine if the player to move was placed into check.
			// The color that just moved must not be in check.
			return !is_king_in_check(this->other_color());
		}

	private:
		void make_move(const rank start_rank, const file start_file, const rank end_rank, const file end_file)
		{
			auto& start = piece_at(start_rank, start_file);
			piece_at(end_rank, end_file) = start;
			start = piece(empty);
		}

		bool board_has_move() const
		{
			// if any of start/end file/rank are valid (not -1), all are valid
			return _start_rank != rank{ -1 };
		}
		const piece& last_moved_piece() const
		{
			return piece_at(_end_rank, _end_file);
		}

		// these are in the same order that they are called in the generation function, which acts in order of probable piece frequency

		void find_pawn_moves(child_boards& child_boards, const rank rank, const file file) const
		{
			if (piece_at(rank, file).is_white())
			{
				if (bounds_check(rank - 1)) // only validate moving forwards once
				{
					// check for moving forward one square
					if (piece_at(rank - 1, file).is_empty())
					{
						if (rank == 1) // if the pawn is on the second last rank
						{
							child_boards.emplace_back(*this, rank, file, rank - 1, file, white_queen);
							child_boards.emplace_back(*this, rank, file, rank - 1, file, white_rook);
							child_boards.emplace_back(*this, rank, file, rank - 1, file, white_bishop);
							child_boards.emplace_back(*this, rank, file, rank - 1, file, white_knight);
						}
						else // the pawn is moving without promotion
						{
							child_boards.emplace_back(*this, rank, file, rank - 1, file);
						}
					}
					// check for moving forward two squares
					if (rank == 6 && piece_at(5, file).is_empty() && piece_at(4, file).is_empty())
					{
						child_boards.emplace_back(*this, rank, file, 4, file);
					}
					// check for captures
					if (bounds_check(file + 1) && piece_at(rank - 1, file + 1).is_occupied() && piece_at(rank - 1, file + 1).is_black())
					{
						if (rank == 1) // if the pawn is on the second last rank
						{
							child_boards.emplace_back(*this, rank, file, rank - 1, file + 1, white_queen);
							child_boards.emplace_back(*this, rank, file, rank - 1, file + 1, white_rook);
							child_boards.emplace_back(*this, rank, file, rank - 1, file + 1, white_bishop);
							child_boards.emplace_back(*this, rank, file, rank - 1, file + 1, white_knight);
						}
						else // the pawn is capturing without promotion
						{
							child_boards.emplace_back(*this, rank, file, rank - 1, file + 1);
						}
					}
					if (bounds_check(file - 1) && piece_at(rank - 1, file - 1).is_occupied() && piece_at(rank - 1, file - 1).is_black())
					{
						if (rank == 1) // if the pawn is on the second last rank
						{
							child_boards.emplace_back(*this, rank, file, rank - 1, file - 1, white_queen);
							child_boards.emplace_back(*this, rank, file, rank - 1, file - 1, white_rook);
							child_boards.emplace_back(*this, rank, file, rank - 1, file - 1, white_bishop);
							child_boards.emplace_back(*this, rank, file, rank - 1, file - 1, white_knight);
						}
						else // the pawn is capturing without promotion
						{
							child_boards.emplace_back(*this, rank, file, rank - 1, file - 1);
						}
					}
					// check for en passant
					if (rank == 3)
					{
						if (en_passant_file == file - 1 && bounds_check(file - 1) && piece_at(rank, file - 1).is_pawn())
							child_boards.emplace_back(*this, rank, file, rank - 1, file - 1);
						else if (en_passant_file == file + 1 && bounds_check(file + 1) && piece_at(rank, file + 1).is_pawn())
							child_boards.emplace_back(*this, rank, file, rank - 1, file + 1);
					}
				}
			}
			else if (piece_at(rank, file).is_black())
			{
				if (bounds_check(rank + 1)) // only validate moving forwards once
				{
					// check for moving forward one square
					if (piece_at(rank + 1, file).is_empty())
					{
						if (rank == 6) // if the pawn is on the second last rank
						{
							child_boards.emplace_back(*this, rank, file, rank + 1, file, black_queen);
							child_boards.emplace_back(*this, rank, file, rank + 1, file, black_rook);
							child_boards.emplace_back(*this, rank, file, rank + 1, file, black_bishop);
							child_boards.emplace_back(*this, rank, file, rank + 1, file, black_knight);
						}
						else // the pawn is moving without promotion
						{
							child_boards.emplace_back(*this, rank, file, rank + 1, file);
						}
					}
					// check for moving forward two squares
					if (rank == 1 && piece_at(2, file).is_empty() && piece_at(3, file).is_empty())
					{
						child_boards.emplace_back(*this, rank, file, 3, file);
					}
					// check for captures
					if (bounds_check(file + 1) && piece_at(rank + 1, file + 1).is_occupied() && piece_at(rank + 1, file + 1).is_white())
					{
						if (rank == 6) // if the pawn is on the second last rank
						{
							child_boards.emplace_back(*this, rank, file, rank + 1, file + 1, black_queen);
							child_boards.emplace_back(*this, rank, file, rank + 1, file + 1, black_rook);
							child_boards.emplace_back(*this, rank, file, rank + 1, file + 1, black_bishop);
							child_boards.emplace_back(*this, rank, file, rank + 1, file + 1, black_knight);
						}
						else // the pawn is capturing without promotion
						{
							child_boards.emplace_back(*this, rank, file, rank + 1, file + 1);
						}
					}
					if (bounds_check(file - 1) && piece_at(rank + 1, file - 1).is_occupied() && piece_at(rank + 1, file - 1).is_white())
					{
						if (rank == 6) // if the pawn is on the second last rank
						{
							child_boards.emplace_back(*this, rank, file, rank + 1, file - 1, black_queen);
							child_boards.emplace_back(*this, rank, file, rank + 1, file - 1, black_rook);
							child_boards.emplace_back(*this, rank, file, rank + 1, file - 1, black_bishop);
							child_boards.emplace_back(*this, rank, file, rank + 1, file - 1, black_knight);
						}
						else // the pawn is capturing without promotion
						{
							child_boards.emplace_back(*this, rank, file, rank + 1, file - 1);
						}
					}
					// check for en passant
					if (rank == 4)
					{
						if (en_passant_file == file - 1 && bounds_check(file - 1) && piece_at(rank, file - 1).is_pawn())
							child_boards.emplace_back(*this, rank, file, rank + 1, file - 1);
						else if (en_passant_file == file + 1 && bounds_check(file + 1) && piece_at(rank, file + 1).is_pawn())
							child_boards.emplace_back(*this, rank, file, rank + 1, file + 1);
					}
				}
			}
		}
		void find_rook_moves(child_boards& child_boards, const rank rank, const file file) const
		{
			// rank descending
			for (chess::rank end_rank = rank - 1; end_rank >= 0; --end_rank)
			{
				if (!bounds_check(end_rank)) break; // out of bounds; don't keep iterating in this direction

				if (piece_at(end_rank, file).is_empty()) // if the square is empty, the rook can move here
				{
					child_boards.emplace_back(*this, rank, file, end_rank, file);
					continue; // keep searching in the current direction
				}
				// if the rook has encountered an enemy piece
				else if (piece_at(end_rank, file).is_opposing_color(color_to_move))
				{
					// the rook can capture...
					child_boards.emplace_back(*this, rank, file, end_rank, file);
					break; // ...but the rook cannot keep moving
				}
				else break; // the rook cannot move into a friendly piece; stop searching this way
			}

			// rank ascending (documentation same as above)
			for (chess::rank end_rank = rank + 1; end_rank < 8; ++end_rank)
			{
				if (!bounds_check(end_rank)) break;

				if (piece_at(end_rank, file).is_empty())
				{
					child_boards.emplace_back(*this, rank, file, end_rank, file);
					continue;
				}
				else if (piece_at(end_rank, file).is_opposing_color(color_to_move))
				{
					child_boards.emplace_back(*this, rank, file, end_rank, file);
					break;
				}
				else break;
			}

			// file descending (documentation same as above)
			for (chess::file end_file = file - 1; end_file >= 0; --end_file)
			{
				if (!bounds_check(end_file)) break;

				if (piece_at(rank, end_file).is_empty())
				{
					child_boards.emplace_back(*this, rank, file, rank, end_file);
					continue;
				}
				else if (piece_at(rank, end_file).is_opposing_color(color_to_move))
				{
					child_boards.emplace_back(*this, rank, file, rank, end_file);
					break;
				}
				else break;
			}

			// file ascending (documentation same as above)
			for (chess::file end_file = file + 1; end_file < 8; ++end_file)
			{
				if (!bounds_check(end_file)) break;

				if (piece_at(rank, end_file).is_empty())
				{
					child_boards.emplace_back(*this, rank, file, rank, end_file);
					continue;
				}
				else if (piece_at(rank, end_file).is_opposing_color(color_to_move))
				{
					child_boards.emplace_back(*this, rank, file, rank, end_file);
					break;
				}
				else break;
			}
		}
		void find_bishop_moves(child_boards& child_boards, const rank rank, const file file) const
		{
			// working diagonally (rank and file descending)
			for (int offset = 1; offset < 8; ++offset)
			{
				// if the location is off of the board, stop searching in this direction
				if (!bounds_check(rank - offset, file - offset)) break;

				// if the square is empty
				if (piece_at(rank - offset, file - offset).is_empty())
				{
					// the bishop can move here
					child_boards.emplace_back(*this, rank, file, rank - offset, file - offset);
					continue; // keep searching in this direction
				}
				// if the square is occupied by an enemy piece, the bishop can capture it
				else if (piece_at(rank - offset, file - offset).is_opposing_color(color_to_move))
				{
					child_boards.emplace_back(*this, rank, file, rank - offset, file - offset);
					// the bishop made a capture, stop searching in this direction
					break;
				}
				// else, the square is occupied by a friendly piece, stop searching in this direction
				else break;
			}

			// working diagonally (rank descending and file ascending) (documentation same as above)
			for (int offset = 1; offset < 8; ++offset)
			{
				if (!bounds_check(rank - offset, file + offset)) break;

				if (piece_at(rank - offset, file + offset).is_empty())
				{
					child_boards.emplace_back(*this, rank, file, rank - offset, file + offset);
					continue;
				}
				else if (piece_at(rank - offset, file + offset).is_opposing_color(color_to_move))
				{
					child_boards.emplace_back(*this, rank, file, rank - offset, file + offset);
					break;
				}
				else break;
			}

			// working diagonally (rank ascending and file descending) (documentation same as above)
			for (int offset = 1; offset < 8; ++offset)
			{
				if (!bounds_check(rank + offset, file - offset)) break;

				if (piece_at(rank + offset, file - offset).is_empty())
				{
					child_boards.emplace_back(*this, rank, file, rank + offset, file - offset);
					continue;
				}
				else if (piece_at(rank + offset, file - offset).is_opposing_color(color_to_move))
				{
					child_boards.emplace_back(*this, rank, file, rank + offset, file - offset);
					break;
				}
				else break;
			}

			// working diagonally (rank and file ascending) (documentation same as above)
			for (int offset = 1; offset < 8; ++offset)
			{
				if (!bounds_check(rank + offset, file + offset)) break;

				if (piece_at(rank + offset, file + offset).is_empty())
				{
					child_boards.emplace_back(*this, rank, file, rank + offset, file + offset);
					continue;
				}
				else if (piece_at(rank + offset, file + offset).is_opposing_color(color_to_move))
				{
					child_boards.emplace_back(*this, rank, file, rank + offset, file + offset);
					break;
				}
				else break;
			}
		}
		void find_knight_moves(child_boards& child_boards, const rank rank, const file file) const
		{
			if (bounds_check(rank - 2, file + 1) && !(piece_at(rank - 2, file + 1).is_occupied() && piece_at(rank - 2, file + 1).is_color(color_to_move)))
				child_boards.emplace_back(*this, rank, file, rank - 2, file + 1);
			if (bounds_check(rank - 1, file + 2) && !(piece_at(rank - 1, file + 2).is_occupied() && piece_at(rank - 1, file + 2).is_color(color_to_move)))
				child_boards.emplace_back(*this, rank, file, rank - 1, file + 2);

			if (bounds_check(rank + 1, file + 2) && !(piece_at(rank + 1, file + 2).is_occupied() && piece_at(rank + 1, file + 2).is_color(color_to_move)))
				child_boards.emplace_back(*this, rank, file, rank + 1, file + 2);
			if (bounds_check(rank + 2, file + 1) && !(piece_at(rank + 2, file + 1).is_occupied() && piece_at(rank + 2, file + 1).is_color(color_to_move)))
				child_boards.emplace_back(*this, rank, file, rank + 2, file + 1);

			if (bounds_check(rank + 2, file - 1) && !(piece_at(rank + 2, file - 1).is_occupied() && piece_at(rank + 2, file - 1).is_color(color_to_move)))
				child_boards.emplace_back(*this, rank, file, rank + 2, file - 1);
			if (bounds_check(rank + 1, file - 2) && !(piece_at(rank + 1, file - 2).is_occupied() && piece_at(rank + 1, file - 2).is_color(color_to_move)))
				child_boards.emplace_back(*this, rank, file, rank + 1, file - 2);

			if (bounds_check(rank - 1, file - 2) && !(piece_at(rank - 1, file - 2).is_occupied() && piece_at(rank - 1, file - 2).is_color(color_to_move)))
				child_boards.emplace_back(*this, rank, file, rank - 1, file - 2);
			if (bounds_check(rank - 2, file - 1) && !(piece_at(rank - 2, file - 1).is_occupied() && piece_at(rank - 2, file - 1).is_color(color_to_move)))
				child_boards.emplace_back(*this, rank, file, rank - 2, file - 1);
		}
		void find_queen_moves(child_boards& child_boards, const rank rank, const file file) const
		{
			find_rook_moves(child_boards, rank, file);
			find_bishop_moves(child_boards, rank, file);
		}
		void find_king_moves(child_boards& child_boards, const rank rank, const file file) const
		{
			// iterate over all adjacent squares
			for (int rank_d = -1; rank_d <= 1; ++rank_d)
			{
				for (int file_d = -1; file_d <= 1; ++file_d)
				{
					// if the square is not occupied by a friendly piece
					if (bounds_check(rank + rank_d, file + file_d) &&
						!(piece_at(rank + rank_d, file + file_d).is_occupied() &&
						  piece_at(rank + rank_d, file + file_d).is_color(color_to_move)))
					{
						child_boards.emplace_back(*this, rank, file, rank + rank_d, file + file_d);
					}
				}
			}

			const piece& king = piece_at(rank, file);

			if ((king.is_white() && white_can_castle_ks) ||
				(king.is_black() && black_can_castle_ks))
			{
				if (// If white can castle to kingside, then we already know the king and rook are in place.
					// Check if the squares in between are empty. 
					piece_at(rank, file + 1).is_empty() && // Check if the squares in between are empty.
					piece_at(rank, file + 2).is_empty() &&
					!is_king_in_check(king, rank, file) && // check if the king is in check now...			
					!other_board_t(*this, rank, file, rank, file + 1).is_king_in_check(king, rank, file + 1) && // ...on his way...
					!other_board_t(*this, rank, file, rank, file + 2).is_king_in_check(king, rank, file + 2)) // ...or at his destination.
				{
					child_boards.emplace_back(*this, rank, file, rank, file + 2); // the board constructor will detect a castle and move the rook as well
				}
			}

			if ((king.is_white() && white_can_castle_qs) || // documentation same as above
				(king.is_black() && black_can_castle_qs))
			{
				if (piece_at(rank, file - 1).is_empty() &&
					piece_at(rank, file - 2).is_empty() &&
					piece_at(rank, file - 3).is_empty() && // we need to check that this square is empty for the rook to move through, but no check test is needed
					!is_king_in_check(king, rank, file) &&
					!other_board_t(*this, rank, file, rank, file - 1).is_king_in_check(king, rank, file - 1) &&
					!other_board_t(*this, rank, file, rank, file - 2).is_king_in_check(king, rank, file - 2))
				{
					child_boards.emplace_back(*this, rank, file, rank, file - 2);
				}
			}
		}

		bool is_king_in_check(const piece king, const rank rank, const file file) const
		{
			bool do_pawn_checks = false;
			bool do_knight_checks = false;
			bool do_king_checks = false;

			// If the last move is known, and a player is starting their turn, some piece checks can be skipped.
			if (board_has_move() && king.is_color(color_to_move))
			{
				const piece last_moved = last_moved_piece();

				// Only look for pawn checks if the last moved piece is a pawn.
				if (last_moved.is_pawn())
					do_pawn_checks = true;
				// Only look for knight checks if the last moved piece is a knight.
				else if (last_moved.is_knight())
					do_knight_checks = true;
				// Only look for king checks if the last moved piece is a king.
				else if (last_moved.is_king())
					do_king_checks = true;
			}
			else
			{
				// We don't know what move created this position, or it is not the king's color's turn to move; do all checks.
				do_pawn_checks = true;
				do_knight_checks = true;
				do_king_checks = true;
			}

			if (do_king_checks)
			{
				// Check adjacent squares for a king.
				// Check rank first, because a king is likely on a top or bottom rank.
				if (bounds_check(rank - 1))
				{
					if (bounds_check(file - 1) && piece_at(rank - 1, file - 1).is_king()) return true;
					if (piece_at(rank - 1, file).is_king()) return true;
					if (bounds_check(file + 1) && piece_at(rank - 1, file + 1).is_king()) return true;
				}

				if (bounds_check(file - 1) && piece_at(rank, file - 1).is_king()) return true;
				if (bounds_check(file + 1) && piece_at(rank, file + 1).is_king()) return true;

				if (bounds_check(rank + 1))
				{
					if (bounds_check(file - 1) && piece_at(rank + 1, file - 1).is_king()) return true;
					if (piece_at(rank + 1, file).is_king()) return true;
					if (bounds_check(file + 1) && piece_at(rank + 1, file + 1).is_king()) return true;
				}
			}

			// iterate in all four vertical and horizontal directions to check for a rook or queen (these loops only look within bounds)

			// rank descending
			for (auto other_rank = rank - 1; other_rank >= 0; --other_rank)
			{
				// if a square is found that is not empty
				if (piece_at(other_rank, file).is_occupied())
				{
					// if the piece is a rook/queen AND is hostile, the king is in check
					if ((piece_at(other_rank, file).is_rook() || piece_at(other_rank, file).is_queen())
						&& piece_at(other_rank, file).is_opposing_color(king)) return true;
					break; // a piece was found in this direction, stop checking in this direction
				}
			}
			// rank ascending (documentation same as above)
			for (auto other_rank = rank + 1; other_rank < 8; ++other_rank)
			{
				if (piece_at(other_rank, file).is_occupied())
				{
					if ((piece_at(other_rank, file).is_rook() || piece_at(other_rank, file).is_queen())
						&& piece_at(other_rank, file).is_opposing_color(king)) return true;
					break;
				}
			}
			// file descending (documentation same as above)
			for (auto other_file = file - 1; other_file >= 0; --other_file)
			{
				if (piece_at(rank, other_file).is_occupied())
				{
					if ((piece_at(rank, other_file).is_rook() || piece_at(rank, other_file).is_queen())
						&& piece_at(rank, other_file).is_opposing_color(king)) return true;
					break;
				}
			}
			// file ascending (documentation same as above)
			for (auto other_file = file + 1; other_file < 8; ++other_file)
			{
				if (piece_at(rank, other_file).is_occupied())
				{
					if ((piece_at(rank, other_file).is_rook() || piece_at(rank, other_file).is_queen())
						&& piece_at(rank, other_file).is_opposing_color(king)) return true;
					break;
				}
			}

			// iterate in all four diagonal directions to find a bishop or queen

			// search rank and file descending
			for (int offset = 1; offset < 8; ++offset)
			{
				// if the coordinates are in bounds
				if (!bounds_check(rank - offset, file - offset)) break;

				// if there is a piece here
				if (piece_at(rank - offset, file - offset).is_occupied())
				{
					// if the piece is a bishop/queen of the opposing color, the king is in check
					if ((piece_at(rank - offset, file - offset).is_bishop() || piece_at(rank - offset, file - offset).is_queen())
						&& piece_at(rank - offset, file - offset).is_opposing_color(king)) return true;
					break; // a piece is here, don't keep searching in this direction
				}
			}

			// search rank descending and file ascending (documentation same as above)
			for (int offset = 1; offset < 8; ++offset)
			{
				if (!bounds_check(rank - offset, file + offset)) break;

				if (piece_at(rank - offset, file + offset).is_occupied())
				{
					if ((piece_at(rank - offset, file + offset).is_bishop() || piece_at(rank - offset, file + offset).is_queen())
						&& piece_at(rank - offset, file + offset).is_opposing_color(king)) return true;
					break;
				}
			}

			// search rank ascending and file descending (documentation same as above)
			for (int offset = 1; offset < 8; ++offset)
			{
				if (!bounds_check(rank + offset, file - offset)) break;

				if (piece_at(rank + offset, file - offset).is_occupied())
				{
					if ((piece_at(rank + offset, file - offset).is_bishop() || piece_at(rank + offset, file - offset).is_queen())
						&& piece_at(rank + offset, file - offset).is_opposing_color(king)) return true;
					break;
				}
			}

			// search rank and file ascending (documentation same as above)
			for (int offset = 1; offset < 8; ++offset)
			{
				if (!bounds_check(rank + offset, file + offset)) break;

				if (piece_at(rank + offset, file + offset).is_occupied())
				{
					if ((piece_at(rank + offset, file + offset).is_bishop() || piece_at(rank + offset, file + offset).is_queen())
						&& piece_at(rank + offset, file + offset).is_opposing_color(king)) return true;
					break;
				}
			}

			if (do_knight_checks)
			{
				const piece_t opposing_knight = other(king.get_color()) | detail::knight;
				if (knight_attacks[to_index(rank, file)](_position, opposing_knight)) return true;
			}

			// check if the white king is under attack by a black pawn
			if (do_pawn_checks)
			{
				if (king.is_white())
				{
					if ((bounds_check(rank - 1, file + 1) && piece_at(rank - 1, file + 1).is(black_pawn)) ||
						(bounds_check(rank - 1, file - 1) && piece_at(rank - 1, file - 1).is(black_pawn))) return true;
				}
				// check if the black king is under attack by a white pawn
				else if (king.is_black())
				{
					if ((bounds_check(rank + 1, file + 1) && piece_at(rank + 1, file + 1).is(white_pawn)) ||
						(bounds_check(rank + 1, file - 1) && piece_at(rank + 1, file - 1).is(white_pawn))) return true;
				}
			}

			return false;
		}

		friend board<white>;
		friend board<black>;
	};

	template<typename board_list>
	void remove_invalid_boards(board_list& boards)
	{
		static board_list valid_boards;

		for (auto& board : boards)
		{
			if (board.is_valid_position())
			{
				valid_boards.push_back(board);
			}
		}

		boards.swap(valid_boards);
		valid_boards.clear();
	}

	void print_board(const auto& board, const unsigned& indent)
	{
		for (rank rank = 0; rank < 8; ++rank)
		{
			// add indent
			for (unsigned i = 0; i < indent * 9; ++i) std::cout << ' ';

			for (file file = 0; file < 8; ++file)
			{
				const piece& piece = board.piece_at(rank, file);

				if (piece.is_empty()) std::cout << ".";
				else if (piece.is_white())
				{
					if (piece.is_rook()) std::cout << "R";
					else if (piece.is_bishop()) std::cout << "B";
					else if (piece.is_knight()) std::cout << "N";
					else if (piece.is_queen()) std::cout << "Q";
					else if (piece.is_king()) std::cout << "K";
				}
				else if (piece.is_black())
				{
					if (piece.is_rook()) std::cout << "r";
					else if (piece.is_bishop()) std::cout << "b";
					else if (piece.is_knight()) std::cout << "n";
					else if (piece.is_queen()) std::cout << "q";
					else if (piece.is_king()) std::cout << "k";
				}
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
	void print_board(const auto& boards)
	{
		for (rank rank = 0; rank < 8; ++rank)
		{
			// render this rank for each board
			for (auto it = boards.cbegin(); it != boards.cend(); ++it)
			{
				for (file file = 0; file < 8; ++file)
				{
					const piece& piece = it->piece_at(rank, file);

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
				}

				// add indent if there is another board to print
				if (it != --boards.cend())
					for (unsigned i = 0; i < 5; ++i) std::cout << ' ';
			}

			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
}
