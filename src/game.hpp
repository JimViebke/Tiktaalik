#pragma once

#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <variant>

#include "move.hpp"
#include "perft.hpp"
#include "search.hpp"
#include "uci.hpp"
#include "util/util.hpp"

namespace chess
{
	const std::string start_pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

	color_t load_fen(const std::string& fen, position& _position);

	void send_command(const std::string& command); // Forward-declare.

	class Game
	{
	public:
		Game() : color_to_move{ load_fen(start_pos, positions[0]) }
		{
			update_info_for_new_root_position();

			std::thread([this] { worker_thread(); }).detach();
		}

		void send_info(const eval_t eval);
		void apply_moves(const std::vector<std::string>& args, size_t move_idx);
		void process_position_command(const std::vector<std::string>& args);
		void process_go_command(const std::vector<std::string>& args);
		void process_uci_commands();

	private:
		void generate_child_boards_for_root()
		{
			size_t end_idx = 0;

			if (color_to_move == white)
				end_idx = generate_child_boards<white>(0);
			else
				end_idx = generate_child_boards<black>(0);

			moves.clear();

			for (size_t idx = first_child_index(0); idx < end_idx; ++idx)
			{
				moves.push_back(boards[idx]);
			}
		}

		void update_info_for_new_root_position()
		{
			generate_child_boards_for_root();
		}

		// Index must be a ply-1 child of the root position.
		// The calling thread must own the game mutex.
		void apply_move(const size_t index)
		{
			// Update root color, position, and board.
			color_to_move = other_color(color_to_move);
			positions[0] = positions[index];
			boards[0] = boards[index];
			++root_ply;
			history[root_ply] = boards[0].get_key();

			// If the PV move was played, the rest of the PV is valid. Shift it up.
			auto& pv = pv_moves[0];
			auto& pv_length = pv_lengths[0];
			if (pv_length > 0 &&
				pv[0].move_to_string() == boards[0].move_to_string())
			{
				std::copy(pv.begin() + 1,
						  pv.begin() + pv_length,
						  pv.begin());
				--pv_length;
			}
			else
			{
				// The PV is no longer valid, mark it as length 0.
				pv_length = 0;
			}

			update_info_for_new_root_position();

			// Decrement the current depth because we're advancing down the tree by one node.
			if (engine_depth > 0)
				--engine_depth;
		}

		// `move` is either in form of "e4f5" or "c7c8q" in the case of promotion.
		// The calling thread must own the game mutex.
		void apply_move(const std::string move)
		{
			// Find and apply the move.
			for (size_t i = first_child_index(0); i < first_child_index(0) + moves.size(); ++i)
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
		}

		template<color_t color_to_move>
		eval_t search(const size_t end_idx, const depth_t depth);

		void worker_thread()
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

				if (moves.size() == 0)
				{
					searching = false;
					util::log("Position is terminal, search thread stopping.");
					continue;
				}

				// Search until we complete another round of iterative deepening,
				// we run out of planned time for this move,
				// or until the main thread stops us.

				engine_start_time = util::time_in_ms();

				const size_t end_idx = first_child_index(0) + moves.size();
				eval_t eval{};
				nodes = 0;
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

					detail::tt.hit = 0;
					detail::tt.miss = 0;
				}
				else if (util::time_in_ms() >= scheduled_turn_end)
				{
					// We stopped searching because we used up the planned time.
					// Send the best move if we have one.
					if (best_move.size() != 0)
					{
						// Play the move internally.
						apply_move(best_move);

						// En Croissant workaround: send the best move as a pv.
						send_command("info pv " + best_move);
						// Play the move.
						send_command("bestmove " + best_move);

						// Ponder after playing the move.
						searching = true;
						pondering = true;
						scheduled_turn_end = util::time_in_ms() + 1'000'000'000;
						util::log("pondering " + best_move);

						best_move = ""; // Reset the best move.
					}
					else
					{
						util::log("Ran out of search time, but no PV move to send (?).");
					}
				}
				else // We were stopped for any other reason.
				{
					best_move = ""; // Reset the best move.
				}
			}
		}

	public:
		void menu()
		{
			while (true)
			{
				std::cout << "\nEnter a command:\n"
					"\tperft [n]\n"
					"\tdivide [n]\n"
					"\tquit\n";

				std::string input;
				std::getline(std::cin, input);
				std::stringstream ss(input);

				std::string command;
				ss >> command;

				if (command == "perft" || command == "divide")
				{
					depth_t depth = 0;
					ss >> depth;
					if (depth < 1)
					{
						depth = 1;
						std::cout << "depth adjusted to " << depth << '\n';
					}
					else if (depth > 10)
					{
						depth = 10;
						std::cout << "depth adjusted to " << depth << '\n';
					}

					if (command == "perft")
					{
						if (color_to_move == white)
							perft<white>(depth);
						else
							perft<black>(depth);
					}
					else if (command == "divide")
					{
						if (color_to_move == white)
							divide<white>(depth);
						else
							divide<black>(depth);
					}
				}
				else if (command == "quit" || command == "q")
				{
					return;
				}
			}
		}

	private:
		std::mutex game_mutex;

		color_t color_to_move;

		depth_t engine_depth = 0;
		size_t n_of_evals = 0;
		util::timepoint engine_start_time = 0;
		util::timepoint engine_time = 0;

		std::vector<board> moves;
	};
}
