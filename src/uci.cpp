
#include <iostream>
#include <sstream>

#include "game.hpp"
#include "perft.hpp"
#include "uci.hpp"
#include "util/util.hpp"

namespace chess
{
	void send_command(const std::string& command)
	{
		{
			std::stringstream ss;
			ss << "Sending UCI command: " << command;
			util::log(ss.str());
		}

		// send the command + endline + sync
		std::cout << command << std::endl;
	}

	void game::send_info(const eval_t eval)
	{
		if (pondering) return; // Don't emit info while pondering.

		engine_time = util::time_in_ms() - engine_start_time;

		std::stringstream ss;
		ss << "info";

		ss << " depth " << engine_depth + 1;

		// Print evaluation in the form "score cp 104" or "score mate -3".
		if (eval >= eval::mate_threshold || eval <= -eval::mate_threshold)
		{
			eval_t plies_to_mate{};
			if (eval >= eval::mate_threshold)
				plies_to_mate = eval::mate - eval + 1; // Count positive plies if white has mate.
			else
				plies_to_mate = -eval::mate - eval - 1; // Count negative plies if black has mate.

			const auto moves_to_mate = plies_to_mate / 2;

			// Flip eval to match engine's perspective per UCI.
			ss << " score mate " << ((color_to_move == white) ? moves_to_mate : moves_to_mate * -1);
		}
		else
		{
			ss << " score cp " << ((color_to_move == white) ? eval : eval * -1);
		}

		ss << " nps " << nodes * 1'000 / std::max(decltype(engine_time)(1), engine_time);
		ss << " nodes " << nodes;
		ss << " hashfull " << tt.occupied_entries * 1'000 / detail::tt_size_in_entries; // Occupancy is per mille.
		ss << " tbhits " << tt.hit;
		ss << " time " << engine_time;

		if (pv_lengths[0] > 0)
		{
			ss << " pv";
			for (size_t i = 0; i < pv_lengths[0]; ++i)
			{
				ss << ' ' << pv_moves[0][i];
			}
		}

		send_command(ss.str());
	}

	void game::apply_moves(const std::vector<std::string>& args, size_t move_idx)
	{
		for (; move_idx < args.size(); ++move_idx)
		{
			apply_move(move{args[move_idx], boards[0].get_bitboards()});
		}
	}

	void game::process_position_command(const std::vector<std::string>& args)
	{
		if (args.size() < 2)
		{
			util::log("Got a position command with no parameters (?).");
			return;
		}

		util::log("Got position command, stopping any search...");
		searching = false;
		util::log("Locking mutex...");
		const std::lock_guard<decltype(game_mutex)> lock(game_mutex);
		pondering = false;
		util::log("Setting up new position.");

		engine_depth = 0;
		engine_time = 0;
		pv_lengths[0] = 0;
		root_ply = 0;

		std::string fen;
		size_t move_token_idx{};
		if (args[1] == "startpos")
		{
			fen = start_pos;
			move_token_idx = 2; // Look for the move token at index 2.
		}
		else if (args[1] == "fen" && args.size() >= 8)
		{
			// Concatenate six args (tokens 2-7).
			fen = args[2];
			for (size_t i = 3; i < 8; ++i)
			{
				fen.push_back(' ');
				fen.append(args[i]);
			}
			move_token_idx = 8; // Look for the move token at index 8.
		}

		color_to_move = boards[0].load_fen(fen);
		generate_child_boards_for_root();

		history[0] = boards[0].get_key();

		if (move_token_idx < args.size() && args[move_token_idx] == "moves")
		{
			apply_moves(args, move_token_idx + 1);
		}
	}

	void game::process_go_command(const std::vector<std::string>& args)
	{
		if (args.size() < 2)
		{
			util::log("Got a go command with no parameters (?).");
			return;
		}

		util::log("Got a go command, stopping any search...");
		searching = false;
		util::log("Locking mutex...");
		const std::lock_guard<decltype(game_mutex)> lock(game_mutex);
		pondering = false;
		util::log("Processing go command.");

		size_t time_left = 0;
		size_t time_inc = 0;
		bool infinite = false;
		bool exact = false;

		for (auto arg_it = args.cbegin() + 1; arg_it != args.cend(); ++arg_it)
		{
			if (*arg_it == "infinite")
			{
				infinite = true;
			}
			// If there is at least one more token after arg_it, check for tokens that expect an argument.
			else if (arg_it + 1 != args.cend())
			{
				if (*arg_it == "movetime")
				{
					exact = true;
					time_left = atoi((arg_it + 1)->c_str());
					continue;
				}
				else if (*arg_it == "perft" || *arg_it == "divide")
				{
					size_t depth = atoi((arg_it + 1)->c_str());
					if (depth > 10)
					{
						depth = 10;
						std::cout << "Capping perft depth to 10.\n";
					}

					if (color_to_move == white)
						divide<white>(depth);
					else
						divide<black>(depth);

					return;
				}

				if (color_to_move == white)
				{
					if (*arg_it == "wtime")
						time_left = atoi((arg_it + 1)->c_str());
					else if (*arg_it == "winc")
						time_inc = atoi((arg_it + 1)->c_str());
				}
				else
				{
					if (*arg_it == "btime")
						time_left = atoi((arg_it + 1)->c_str());
					else if (*arg_it == "binc")
						time_inc = atoi((arg_it + 1)->c_str());
				}
			}
		}

		if (exact)
		{
			scheduled_turn_end = util::time_in_ms() + time_left;
		}
		else if (infinite)
		{
			// Using infinite == ~11 days.
			scheduled_turn_end = util::time_in_ms() + 1'000'000'000;
		}
		else
		{
			if (time_left == 0)
			{
				util::log("Got a go command without time remaining.");
				return;
			}

			// Decide how long to spend searching on this turn.

			// Start with the increment.
			size_t search_ms = time_inc;

			// If we can remove the increment from the clock, do so.
			if (time_left > time_inc) time_left -= time_inc;

			// Add 1/25 of the time remaining.
			search_ms += time_left / 25; // todo: Adjust based on the move number.

			// Use at least one second.
			search_ms = std::max(search_ms, 1'000ull);

			// Use at most half of our remaining clock.
			search_ms = std::min(search_ms, time_left / 2);

			// Note the time point at which to stop searching.
			scheduled_turn_end = util::time_in_ms() + search_ms;
		}

		// Currently, we only update the PV when we finish a round of Iterative Deepening.
		// Reset the engine's depth to make sure we update this.
		engine_depth = 0;

		// Awaken the search thread.
		searching = true;
		searching.notify_one();
	}

	void game::process_uci_commands()
	{
		std::string command;
		while (std::getline(std::cin, command, '\n'))
		{
			if (command.size() == 0)
			{
				util::log("Empty command, ignoring.");
				continue;
			}

			{
				std::stringstream ss;
				ss << "Got command: " << command;
				util::log(ss.str());
			}

			const std::vector<std::string> args = util::tokenize(command);

			if (args[0] == "uci")
			{
				send_command("id name Tiktaalik");
				send_command("id author Jim Viebke");
				send_command("uciok");
			}
			else if (args[0] == "isready")
			{
				send_command("readyok");
			}
			else if (args[0] == "setoption") // setoption name some_name [value some_value]
			{
			}
			else if (args[0] == "position") // position (startpos | (fen fenstring)) [moves ...]
			{
				process_position_command(args);
			}
			else if (args[0] == "go")
			{
				process_go_command(args);
			}
			else if (args[0] == "stop")
			{
				searching = false;
				const std::lock_guard<decltype(game_mutex)> lock(game_mutex);
				pondering = false;
			}
			else if (args[0] == "quit")
			{
				break;
			}
			else if (args[0] == "tune")
			{
#if tuning
				tune(args);
#else
				std::cout << "Tuning not enabled.\n";
#endif
			}
			else
			{
				util::log("(command unrecognized or invalid)");
			}
		}

		util::log("Leaving process_uci_commands()");
	}
}
