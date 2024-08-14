/* 2021 December 16 */

#include "bitboard.hpp"
#include "game.hpp"
#include "move.hpp"
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

	void game::apply_move(const size_t index)
	{
		// Update root color and board.
		color_to_move = other_color(color_to_move);
		boards[0] = boards[index];
		++root_ply;
		history[root_ply] = boards[0].get_key();

		// If the PV move was played, the rest of the PV is valid. Shift it up.
		auto& pv = pv_moves[0];
		auto& pv_length = pv_lengths[0];
		if (pv_length > 0 && pv[0].move_to_string() == boards[0].move_to_string())
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

	void game::apply_move(const std::string& move)
	{
		// Find and apply the move.
		for (size_t i = first_child_index(0); i < first_child_index(0) + n_legal_moves; ++i)
		{
			if (boards[i].move_to_string() == move)
			{
				apply_move(i);
				return;
			}
		}

		// If execution reaches here, we didn't find the move because it is not legal.
		std::stringstream ss;
		ss << "Illegal move: [" << move << ']';
		util::log(ss.str());
		std::cout << ss.str() << '\n';
	}

	void game::send_move(const std::string& move)
	{
		send_command("info pv " + move); // En Croissant workaround: try send the best move as a pv.
		send_command("bestmove " + move);

		// Ponder after playing the move.
		searching = true;
		pondering = true;
		scheduled_turn_end = util::time_in_ms() + 1'000'000'000;
		util::log("pondering " + move);
	}

	template <color color_to_move>
	eval_t game::search(const size_t end_idx, const depth_t depth)
	{
		++nodes;

		eval_t alpha = -eval::mate;
		eval_t beta = eval::mate;
		eval_t eval = (color_to_move == white ? -eval::mate : eval::mate);

		eval_t tt_eval = 0; // ignored
		packed_move tt_move = 0;
		tt.probe(tt_eval, tt_move, boards[0].get_key(), depth, alpha, beta, 0);

		const size_t begin_idx = first_child_index(0);

		swap_tt_move_to_front(tt_move, begin_idx, end_idx);

		for (size_t child_idx = begin_idx; child_idx != end_idx; ++child_idx)
		{
			eval_t ab = alpha_beta<other_color(color_to_move)>(child_idx, 1, depth - 1, alpha, beta);

			if (!searching) return eval;

			if constexpr (color_to_move == white)
			{
				if (ab > eval)
				{
					eval = ab;
					update_pv(0, boards[child_idx]);
					send_info(eval);
					tt_move = boards[child_idx].get_packed_move();
				}
				alpha = std::max(alpha, eval);
			}
			else
			{
				if (ab < eval)
				{
					eval = ab;
					update_pv(0, boards[child_idx]);
					send_info(eval);
					tt_move = boards[child_idx].get_packed_move();
				}
				beta = std::min(beta, eval);
			}

			swap_best_to_front<color_to_move>(child_idx + 1, end_idx);
		}

		// Store the best move in the TT.
		tt.store(boards[0].get_key(), depth, tt_eval_type::exact, eval, 0, tt_move);

		return eval;
	}

	void game::worker_thread()
	{
		// Sleep until the main thread sets searching to true.
		util::log("Worker started, waiting for search to start.");
		searching.wait(false);
		util::log("Worker starting search, getting initial mutex lock...");
		std::unique_lock<decltype(game_mutex)> lock(game_mutex); // constructs and locks
		util::log("Worker ready.");

		std::string best_move;

		while (1)
		{
			if (!searching)
			{
				// Stop searching and release the mutex until the main thread tells us to resume.
				util::log("Worker stopped searching, releasing mutex...");
				lock.unlock();
				util::log("Worker released mutex, sleeping.");
				searching.wait(false);
				util::log("Worker awakened, locking mutex...");
				lock.lock();
				util::log("Worker locked mutex.");

				// Since we've just woken up, assume we know nothing about the position until
				// we learn otherwise.
				best_move = "";
			}

			if (n_legal_moves == 0)
			{
				searching = false;
				pondering = false;
				util::log("Position is terminal, search thread stopping.");
				continue;
			}

			// If there is only one legal move, and it's our turn, play it.
			if (n_legal_moves == 1 && !pondering)
			{
				const std::string move = boards[first_child_index(0)].move_to_string();
				util::log("Playing only legal move: " + move);
				apply_move(first_child_index(0));
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
				if (pv_lengths[0] > 0)
				{
					best_move = pv_moves[0][0].move_to_string();
				}
				else // We stopped searching before finding any line.
				{
					best_move = "";
				}

				++engine_depth;

				util::log(std::format("Finished depth {}, {} ms, {} nodes.", engine_depth, engine_time, nodes));

				// Move immediately if we've found mate and it's our turn.
				if (eval::found_mate(eval) && !pondering && best_move != "")
				{
					util::log("Found mate.");
					apply_move(best_move);
					send_move(best_move);
					best_move = "";
				}
				else if (engine_depth >= depth_t{max_ply})
				{
					if (pondering)
					{
						util::log("Reached max ply while pondering. Stopping.");
					}
					else if (best_move != "")
					{
						apply_move(best_move);
						send_move(best_move);
						util::log("Reached max ply while searching, and played best move. Stopping.");
					}
					else
					{
						util::log("Reached max ply while searching, but no best move to play?");
					}

					best_move = "";
					pondering = false;
					searching = false;
				}
			}
			else if (util::time_in_ms() >= scheduled_turn_end)
			{
				// We stopped searching because we used up the planned time.
				// Send the best move if we have one.
				if (best_move.size() != 0)
				{
					apply_move(best_move);
					send_move(best_move);
					best_move = "";
				}
				else
				{
					util::log("Ran out of search time, but no PV move to send (?).");
				}
			}
			else // We were stopped by the main thread.
			{
				best_move = "";
			}
		}
	}

	template eval_t game::search<white>(const size_t, depth_t);
	template eval_t game::search<black>(const size_t, depth_t);
}
