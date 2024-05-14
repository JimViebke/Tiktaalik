#pragma once

#include <iostream>
#include <map>
#include <variant>
#include <vector>

#include "board.hpp"
#include "types.hpp"
#include "util/util.hpp"

namespace chess
{
	template<color_t color_to_move>
	class node : public colorable<color_to_move>
	{
	public:
		using other_node_t = node<other(color_to_move)>;

		std::vector<other_node_t> children;
		board _board;

		using node_mask_t = uint8_t;

		explicit node(const board set_board) : _board(set_board) {}

		void generate_static_eval(const position& position)
		{
			static_eval = position.evaluate_position();
		}

		void generate_incremental_static_eval(const position& parent_position, const eval_t parent_static_eval, const board& child_board)
		{
			const rank start_rank = child_board.start_rank();
			const file start_file = child_board.start_file();
			const rank end_rank = child_board.end_rank();
			const file end_file = child_board.end_file();

			const size_t start_idx = to_index(start_rank, start_file);
			const size_t end_idx = to_index(end_rank, end_file);

			static_eval = parent_static_eval;

			const piece piece_before = parent_position.piece_at(start_idx);
			static_eval -= eval::piece_eval(piece_before);
			static_eval -= eval::piece_square_eval(piece_before, start_idx);

			const piece piece_after = child_board.moved_piece<this->color_to_move()>(); // will be a different type if promoting
			static_eval += eval::piece_eval(piece_after);
			static_eval += eval::piece_square_eval(piece_after, end_idx);

			// check for en passant captures
			if (parent_position.piece_at(start_idx).is_pawn() &&
				start_file != end_file && // if a pawn captures...
				parent_position.piece_at(end_idx).is_empty()) // ...into an empty square, it must be an en passant capture
			{
				const size_t captured_piece_idx = to_index(start_rank, end_file); // the captured pawn will have the moving pawn's start rank and end file
				const piece captured_piece = parent_position.piece_at(captured_piece_idx);

				static_eval -= eval::piece_eval(captured_piece);
				static_eval -= eval::piece_square_eval(captured_piece, captured_piece_idx);
			}
			// if a king is castling
			else if (parent_position.piece_at(start_idx).is_king() &&
					 diff(start_file, end_file) > 1)
			{
				// update evaluation for the moving rook
				size_t rook_start_idx = 0;
				size_t rook_end_idx = 0;

				if (start_file < end_file) // kingside castle
				{
					rook_start_idx = to_index(start_rank, 7);
					rook_end_idx = to_index(start_rank, 5);
				}
				else // queenside castle
				{
					rook_start_idx = to_index(start_rank, 0);
					rook_end_idx = to_index(start_rank, 3);
				}

				const piece moving_rook = parent_position.piece_at(rook_start_idx);
				static_eval -= eval::piece_square_eval(moving_rook, rook_start_idx);
				static_eval += eval::piece_square_eval(moving_rook, rook_end_idx);
			}

			// check for captures
			const piece captured_piece = parent_position.piece_at(end_idx);
			if (captured_piece.is_occupied())
			{
				static_eval -= eval::piece_eval(captured_piece);
				static_eval -= eval::piece_square_eval(captured_piece, end_idx);
			}
		}

		void set_eval(const eval_t set_eval) { eval = set_eval; }
		eval_t get_eval() const { return eval; }

		eval_t get_static_eval() const { return static_eval; }

		void generate_child_boards(const position& position);
		void clear_node()
		{
			children.clear();
			clear_has_generated_children();
		}

		bool is_terminal() const
		{
			// Anything other than "unknown" is a terminal (end) state.
			return _board.get_result() != result::unknown;
		}

		eval_t terminal_eval()
		{
			const result result = _board.get_result();
			if (result == result::white_wins_by_checkmate)
				eval = eval::eval_max;
			else if (result == result::black_wins_by_checkmate)
				eval = eval::eval_min;
			else if (result == result::draw_by_stalemate)
				eval = 0;
			else
				std::cout << "Unknown terminal state: [" << size_t(result) << "]\n";
			return eval;
		}

		void perft(const position& position, const depth_t max_depth);
		void divide(const position& position, const depth_t max_depth);

	private:
		static constexpr node_mask_t generated_children = 1 << 0;
		static constexpr node_mask_t generated_static_eval = 1 << 1;

		bool has_generated_children() const { return node_mask & generated_children; }

		void set_has_generated_children() { node_mask |= generated_children; }
		void clear_has_generated_children() { node_mask &= ~generated_children; }

		eval_t eval = 0;
		eval_t static_eval = 0;

		node_mask_t node_mask = 0;

		// inner perft
		void perft(const position& current_position,
				   const depth_t depth,
				   const depth_t max_depth,
				   std::map<depth_t, size_t>& node_counter);

		friend node<white>;
		friend node<black>;
	};

	using root_v = std::variant<node<white>, node<black>>;
	using best_move_v = std::variant<node<white>*, node<black>*>;
}
