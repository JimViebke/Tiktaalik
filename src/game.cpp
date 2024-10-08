/* 2021 December 16 */

#include "bitboard.hpp"
#include "game.hpp"
#include "movegen.hpp"
#include "search.hpp"

namespace chess
{
	const std::string start_pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

	game::game() : color_to_move{boards[0].load_fen(start_pos)}
	{
#if tuning
		load_weights();
#endif
		generate_child_boards_for_root();

		std::thread([this] { worker_thread(); }).detach();
	}

	void game::generate_child_boards_for_root()
	{
		size_t end_idx = 0;

		if (color_to_move == white)
			end_idx = generate_child_boards<white>(0);
		else
			end_idx = generate_child_boards<black>(0);

		n_legal_moves = end_idx - first_child_index(0);
	}

	void game::apply_move(const board& board)
	{
		// Update root color and board.
		color_to_move = other_color(color_to_move);
		boards[0] = board;
		++root_ply;
		history[root_ply] = boards[0].get_key();

		// If the PV move was played, the rest of the PV is valid. Shift it up.
		auto& pv = pv_moves[0];
		auto& pv_length = pv_lengths[0];
		if (pv_length > 0 && boards[0].move_is(pv[0]))
		{
			std::copy(pv.begin() + 1, pv.begin() + pv_length, pv.begin());
			--pv_length;
		}
		else
		{
			// The PV is no longer valid, mark it as length 0.
			pv_length = 0;
		}

		generate_child_boards_for_root();

		// Decrement the current depth because we're advancing down the tree by one node.
		if (engine_depth > 0) --engine_depth;
	}

	void game::apply_move(const move move)
	{
		// Find and apply the move.
		for (size_t i = first_child_index(0); i < first_child_index(0) + n_legal_moves; ++i)
		{
			if (boards[i].move_is(move))
			{
				apply_move(boards[i]);
				return;
			}
		}

		// If execution reaches here, we didn't find the move because it is not legal.
		std::stringstream ss;
		ss << "Illegal move: [" << move << ']';
		util::log(ss.str());
		std::cout << ss.str() << '\n';
	}

	void game::send_move(const move move)
	{
		const auto move_string = move.to_string();
		send_command("bestmove " + move_string);

		if (ponder_enabled)
		{
			// Ponder after playing the move.
			searching = true;
			pondering = true;
			scheduled_turn_end = util::time_in_ms() + 1'000'000'000; // ~11 days
			util::log("Pondering.");
		}
		else
		{
			searching = false;
			pondering = false;
			util::log("Stopping.");
		}
	}

	template <color color_to_move>
	eval_t game::search(const size_t end_idx, const depth_t depth)
	{
		++nodes;

		eval_t alpha = -eval::mate;
		eval_t beta = eval::mate;
		eval_t eval = -eval::mate;

		eval_t tt_eval{}; // ignored
		move tt_move{};
		tt.probe(tt_eval, tt_move, boards[0].get_key(), depth, alpha, beta, 0);

		const size_t begin_idx = first_child_index(0);

		swap_tt_move_to_front(tt_move, begin_idx, end_idx);

		for (size_t child_idx = begin_idx; child_idx != end_idx; ++child_idx)
		{
			const eval_t ab = -alpha_beta<other_color(color_to_move)>(child_idx, 1, depth - 1, -beta, -alpha);

			if (!searching) return eval;

			if (ab > eval)
			{
				eval = ab;
				update_pv(0, boards[child_idx]);
				send_info(eval * (color_to_move == white ? 1 : -1));
				tt_move = boards[child_idx].get_move();
			}
			alpha = std::max(alpha, eval);

			swap_best_to_front<color_to_move>(child_idx + 1, end_idx);
		}

		// Store the best move in the TT.
		tt.store(boards[0].get_key(), depth, tt_eval_type::exact, eval, 0, tt_move);

		return eval;
	}

	void game::worker_thread()
	{
		// Sleep until the main thread wakes us.
		searching.wait(false);
		std::unique_lock<decltype(game_mutex)> lock(game_mutex);
		util::log("Worker started.");

		while (1)
		{
			if (!searching)
			{
				// Stop searching and release the mutex until the main thread tells us to resume.
				util::log("Worker stopped.");
				lock.unlock();
				searching.wait(false);
				lock.lock();
				util::log("Worker resumed.");
			}

			if (n_legal_moves == 0)
			{
				searching = false;
				pondering = false;
				util::log("Position is terminal.");
				continue;
			}

			// If there is only one legal move, and it's our turn, play it.
			if (n_legal_moves == 1 && !pondering)
			{
				const move move = boards[first_child_index(0)].get_move();
				util::log("Playing only legal move: " + move.to_string());
				apply_move(move);
				send_move(move);
				continue;
			}

			// Search until we complete another round of iterative deepening,
			// we run out of planned time for this move,
			// or until the main thread stops us.

			engine_start_time = util::time_in_ms();

			const size_t end_idx = first_child_index(0) + n_legal_moves;
			nodes = 0;
			tt.hit = 0;
			tt.miss = 0;

			util::log(std::format("Engine depth {}, searching depth {}.", engine_depth, engine_depth + 1));
			eval_t eval = 0;
			if (color_to_move == white)
				eval = search<white>(end_idx, engine_depth + 1);
			else
				eval = search<black>(end_idx, engine_depth + 1);

			engine_time = util::time_in_ms() - engine_start_time;

			// If searching is still true, we finished another round of iterative deepening.
			if (searching)
			{
				++engine_depth;

				util::log(std::format("Finished depth {} in {} ms, {} nodes.", engine_depth, engine_time, nodes));

				// Move immediately if we've found mate and it's our turn.
				if (eval::found_mate(eval) && !pondering)
				{
					util::log("Found mate.");

					move move{};
					if (pv_lengths[0] > 0)
					{
						move = pv_moves[0][0];
					}
					else
					{
						move = boards[first_child_index(0)].get_move();
						util::log("Error: found mate, but no PV move.");
					}

					apply_move(move);
					send_move(move);
				}
				else if (engine_depth >= depth_t{max_ply})
				{
					if (pondering)
					{
						util::log("Reached max ply while pondering. Stopping.");
					}
					else
					{
						const move move = pv_moves[0][0];
						apply_move(move);
						send_move(move);
						util::log("Reached max ply while searching, and played best move. Stopping.");
					}

					pondering = false;
					searching = false;
				}
			}
			else if (util::time_in_ms() >= scheduled_turn_end)
			{
				// We stopped searching because we used up the planned time.
				// Play the best move we have.
				move move{};
				if (pv_lengths[0] > 0)
				{
					move = pv_moves[0][0];
				}
				else
				{
					move = boards[first_child_index(0)].get_move();
					util::log("Error: ran out of search time, but no PV move.");
				}

				apply_move(move);
				send_move(move);
			}
			else // We were stopped by the main thread.
			{
			}
		}
	}

	template eval_t game::search<white>(const size_t, depth_t);
	template eval_t game::search<black>(const size_t, depth_t);
}
