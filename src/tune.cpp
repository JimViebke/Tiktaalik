
#include <array>
#include <charconv>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "board.hpp"
#include "game.hpp"
#include "search.hpp"

namespace chess
{
#if tuning
	const std::string base_dir = "D:/Games/chess/";

	const std::string weights_file = base_dir + "engines/tiktaalik weights.txt";
	const std::string formatted_weights_files = base_dir + "engines/tiktaalik weights formatted.txt";
	const std::string games_file = base_dir + "games/CCRL-4040.[1913795]-rated-2500-no-mate.uci";

	std::vector<board> test_positions;
	std::vector<color> side_to_move;
	std::vector<float> results;
#endif

#if tuning
	// ps_evals is usually static constexpr, and visible from evaluation.hpp.
	// During tuning, instead provide it here.
	namespace eval
	{
		std::array<int16_t, pse_size> ps_evals;
	}
#endif

#if tuning
	using namespace eval;
#endif

#if tuning
	bool load_weights()
	{
		std::cout << "Loading weights from " << weights_file << '\n';

		std::fstream file(weights_file);

		std::string weights;
		std::string line;
		while (std::getline(file, line))
		{
			if (line.size() > 0)
			{
				weights = line;
			}
		}

		std::stringstream ss{weights};
		eval_t eval = 0;
		for (size_t i = 0; i < eval::ps_evals.size(); ++i)
		{
			if (ss >> eval)
			{
				eval::ps_evals[i] = eval;
			}
			else
			{
				std::cout << std::format("Failed to read weight for ps_evals[{}].\n", i);
				return false;
			}
		}

		if (ss >> eval)
		{
			std::cout << "Found at least one extra weight (?).\n";
			return false;
		}

		std::cout << std::format("Loaded {} weights.\n", eval::ps_evals.size());
		return true;
	}
#endif

#if tuning
	static void store_weights()
	{
		std::stringstream ss;

		ss << int(eval::ps_evals[0]);
		for (size_t i = 1; i < ps_evals.size(); ++i)
		{
			ss << ' ' << int(ps_evals[i]);
		}
		ss << '\n';

		std::ofstream file(weights_file, std::ios_base::app);
		file << ss.str();
	}
#endif

#if tuning
	void game::load_games()
	{
		const auto start_time = util::time_in_ms();

		// Make sure we're starting in a clean state.
		root_ply = 0;
		color_to_move = boards[0].load_fen(start_pos);
		const board start_board = boards[0];
		generate_child_boards_for_root();

		// Seed rng with a constant so we select a deterministic set of positions
		// given the same set of games.
		std::mt19937_64 rng{0xcafe0123456789};

		std::ifstream fs(games_file);
		std::cout << "Loading games from " << games_file << ".\n";

		size_t games_parsed = 0;

		std::string line;
		while (std::getline(fs, line))
		{
			// Skip empty lines.
			if (line.size() == 0) continue;
			// Skip lines containing tags.
			if (line[0] == '[') continue;

			std::vector<std::string> moves = util::tokenize(line);

			if (moves.size() < 1)
			{
				std::cout << "Tried to tokenize a line, but got nothing. Stopping.\n";
				std::cout << "line: [" << line << "]\n";
				return;
			}

			if (moves.size() < 29)
			{
				std::cout << "Line is " << moves.size()
				          << " tokens long. Expected minimum is 28 ply + 1 result. Stopping.\n";
				std::cout << "line: [" << line << "]\n";
				return;
			}

			// Save the result.
			const std::string result = *moves.rbegin();
			if (result == "1-0")
			{
				results.push_back(1);
			}
			else if (result == "0-1")
			{
				results.push_back(0);
			}
			else if (result == "1/2-1/2")
			{
				results.push_back(0.5);
			}
			else
			{
				std::cout << "Unknown result [" << result << "], stopping.\n";
				return;
			}

			// Remove the result token.
			moves.pop_back();

			// Fix pgn-extract outputting UCI promotions as uppercase.
			for (auto& move : moves)
				util::to_lower(move);

			// Reset the game state to the start position.
			root_ply = 0;
			boards[0] = start_board;
			color_to_move = white;
			generate_child_boards_for_root();

			// Get a position from the game by appling a random number of moves between 13 and N inclusive.
			const int position = std::uniform_int_distribution(13, int(moves.size()))(rng);
			for (int i = 0; i < position; ++i)
			{
				apply_move(moves[i]);
			}

			if constexpr (config::verify_key_and_eval)
			{
				boards[0].verify_key_and_eval(color_to_move);
			}

			// Save the position, and the color to move.
			test_positions.push_back(boards[0]);
			side_to_move.push_back(color_to_move);

			++games_parsed;

			if (games_parsed % 1000 == 0)
			{
				std::cout << games_parsed << '\n';
			}
		}

		std::cout << std::format(
		    "Parsed {} games in {} seconds.\n", games_parsed, (util::time_in_ms() - start_time) / 1'000);
	}
#endif

#if tuning
	static void store_weights_formatted()
	{
		std::stringstream ss;
		ss << "ps_evals = {\n";

		for (size_t i = 0; i < ps_evals.size(); ++i)
		{
			if (i % 64 == 0)
			{
				/*
				64 * 0 -> "// pawn midgame"
				64 * 1 -> "// pawn endgame"
				64 * 2 -> "// knight midgame"
				64 * 3 -> "// knight endgame"
				64 * 4 -> "// bishop midgame"
				64 * 5 -> "// bishop endgame"
				...
				64 * 10 -> "// king midgame"
				64 * 11 -> "// king endgame"
				*/

				switch (i / 128)
				{
				case pawn: ss << "// pawn "; break;
				case knight: ss << "// knight "; break;
				case bishop: ss << "// bishop "; break;
				case rook: ss << "// rook "; break;
				case queen: ss << "// queen "; break;
				case king: ss << "// king "; break;
				}

				if (i % 128 == 0)
					ss << "midgame:\n";
				else
					ss << "endgame:\n";
			}

			ss << std::setw(5);
			ss << std::right;
			ss << int(eval::ps_evals[i]) << ',';

			if (i % 8 == 7) ss << "   //\n";
		}

		ss << "};\n";

		std::ofstream ofs(formatted_weights_files, std::ios_base::out);
		ofs << ss.str();
	}
#endif

#if tuning
	/*constexpr*/ double K = 0.4689;

	static_assert(sizeof(double) == 8);
#endif

#if tuning
	static double sigmoid(const double eval)
	{
		const double exp = -K * eval / 400;
		return 1 / (1 + pow(10, exp));
	}
#endif

#if tuning
	// Get the mean squared error for a quiescence search of all test positions.
	static double evaluate()
	{
		double error_sum = 0.0;

		for (size_t i = 0; i < test_positions.size(); ++i)
		{
			boards[0] = test_positions[i];
			boards[0].generate_eval();

			if constexpr (config::verify_key_and_eval)
			{
				boards[0].verify_key_and_eval(side_to_move[i]);
			}

			const eval_t eval = (side_to_move[i] == white) ? alpha_beta<white, true>(0, 0, 0, -eval::mate, eval::mate)
			                                               : alpha_beta<black, true>(0, 0, 0, -eval::mate, eval::mate);

			const double error = results[i] - sigmoid((double)eval);
			error_sum += (error * error);
		}

		return error_sum / test_positions.size();
	}
#endif

#if tuning
	static void tune_k()
	{
		// double best_error = evaluate();

		//	double adjust = 1.0;
		//	while (adjust > 0.000000000000001)
		//	{
		//		for (const double delta : {adjust, -adjust})
		//		{
		//			K += delta;
		//			double new_error = evaluate();

		//			if (new_error < best_error)
		//			{
		//				best_error = new_error;
		//				std::cout << "K = " << K << ", error = " << best_error << '\n';
		//				break;
		//			}

		//			K -= delta;
		//		}

		//		adjust /= 2;
		//	}

		for (int i = 4600; i <= 4800; ++i)
		{
			K = double(i) / 10'000.0;
			std::cout << std::setprecision(15) << "( " << K << ", " << evaluate() << ")\n";
		}
	}
#endif

#if tuning
	static void tune_ps_evals()
	{
		// Make sure we won't exit alpha_beta early.
		scheduled_turn_end = util::time_in_ms() + 1'000'000'000;

		std::cout << "Tuning piece-square evals.\n";

		double best_error = evaluate();
		std::cout << std::format("Starting error: {:1.9f}\n", best_error);

		bool improving = true;
		while (improving)
		{
			improving = false;
			for (size_t i = 0; i < eval::ps_evals.size(); ++i)
			{
				for (int8_t delta : {1, -1})
				{
					eval::ps_evals[i] += delta;

					const double new_error = evaluate();
					if (new_error < best_error)
					{
						store_weights();
						store_weights_formatted();
						std::cout << std::format(
						    "Improved ps_evals[{:>3}] from {:>4} to {:>4}. Old error: {:1.9f}, new error: {:1.9f}\n", i,
						    int(eval::ps_evals[i] - delta), int(eval::ps_evals[i]), best_error, new_error);
						best_error = new_error;
						improving = true;
						break;
					}

					eval::ps_evals[i] -= delta;
				}
			}
		}

		std::cout << "Finished tuning.\n";
	}
#endif

#if tuning
	static void show_tune_help()
	{
		std::cout << "\ttune help\n";
		std::cout << "\ttune evals\n";
		std::cout << "\ttune k\n";
	}
#endif

#if tuning
	void game::tune(const std::vector<std::string>& args)
	{
		if (args.size() < 2 || args[1] == "help")
		{
			show_tune_help();
			return;
		}

		if (args[1] == "k")
		{
			load_games();
			tune_k();
		}
		else if (args[1] == "evals")
		{
			load_games();
			tune_ps_evals();
		}
		else
		{
			std::cout << "Tune command unrecognized/invalid.\n";
			show_tune_help();
		}
	}
#endif
}
