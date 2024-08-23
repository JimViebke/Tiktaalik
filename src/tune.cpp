
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

	struct extended_position
	{
		board board;
		color side_to_move;
		float result;
		double error = -2.0;
	};

	std::vector<extended_position> extended_positions;

	std::vector<extended_position> training_set;
	std::vector<extended_position> test_set;
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
	static void store_weights_formatted()
	{
		constexpr std::string left_indent = "\t";
		constexpr std::string left_align = "  ";

		std::stringstream ss;

		ss << left_indent << "// clang-format off\n";
		ss << left_indent << "constexpr std::array<int16_t, pse_size> ps_evals = {\n";

		for (size_t i = 0; i < ps_evals.size(); ++i)
		{
			// If we're starting a new piece type, add an extra newline.
			if (i % 128 == 0) ss << '\n';

			// If we're starting a new piece-square table, note the type.
			if (i % 64 == 0)
			{
				ss << left_indent;

				/*
				64 * 0 -> "// pawn midgame:"
				64 * 1 -> "// pawn endgame:"
				64 * 2 -> "// knight midgame:"
				64 * 3 -> "// knight endgame:"
				...
				64 * 10 -> "// king midgame:"
				64 * 11 -> "// king endgame:"
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

			// If we're starting a new line, indent and align.
			if (i % 8 == 0)
			{
				ss << left_indent << left_align;
			}

			ss << std::setw(5);
			ss << std::right;
			ss << int(eval::ps_evals[i]) << ',';

			if (i % 8 == 7) ss << '\n';
		}

		ss << left_indent << "};\n";
		ss << left_indent << "// clang-format on\n";

		std::ofstream ofs(formatted_weights_files, std::ios_base::out);
		ofs << ss.str();
	}
#endif

#if tuning
	void game::load_games()
	{
		if (extended_positions.size() != 0)
		{
			std::cout << std::format("{} positions already loaded.\n", extended_positions.size());
			return;
		}

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

			extended_positions.emplace_back();

			// Save the result.
			const std::string result = *moves.rbegin();
			if (result == "1-0")
			{
				extended_positions.back().result = 1;
			}
			else if (result == "0-1")
			{
				extended_positions.back().result = 0;
			}
			else if (result == "1/2-1/2")
			{
				extended_positions.back().result = 0.5;
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

			if constexpr (config::verify_key_phase_eval)
			{
				boards[0].verify_key_phase_eval(color_to_move);
			}

			// Save the position and side to move.
			extended_positions.back().board = boards[0];
			extended_positions.back().side_to_move = color_to_move;

			++games_parsed;

			if (games_parsed % 1'000 == 0)
			{
				std::cout << games_parsed << '\n';
			}
		}

		std::cout << std::format(
		    "Parsed {} games in {} seconds.\n", games_parsed, (util::time_in_ms() - start_time) / 1'000);
	}
#endif

#if tuning
	static void generate_training_and_test_sets(size_t set_size, std::mt19937_64& rng)
	{
		if (extended_positions.size() == 0)
		{
			std::cout << "No positions to select from.\n";
			return;
		}

		if (set_size * 2 > extended_positions.size())
		{
			std::cout << "Not enough positions to select unique training and testing positions.\n";
			return;
		}

		const auto start = util::time_in_ms();

		std::cout << std::format("Selecting {} training positions and {} test positions from {} loaded positions... ",
		    set_size, set_size, extended_positions.size());

		// Make sure we're starting in a clean state.
		training_set.clear();
		test_set.clear();
		training_set.reserve(set_size);
		test_set.reserve(set_size);

		// Generate indexes 0 through extended_positions.size() - 1.
		std::vector<uint32_t> indexes;
		indexes.reserve(extended_positions.size());
		for (size_t i = 0; i < extended_positions.size(); ++i)
		{
			indexes.push_back(i);
		}

		std::shuffle(indexes.begin(), indexes.end(), rng);

		for (size_t i = 0; i < set_size * 2; i += 2)
		{
			training_set.push_back(extended_positions[indexes[i]]);
			test_set.push_back(extended_positions[indexes[i + 1]]);
		}

		std::cout << std::format("done ({} ms)\n", util::time_in_ms() - start);
	}
#endif

#if tuning
	/*constexpr*/ double K = 0.7644;

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
	// Get the mean squared error for a quiescence search of all positions in a set.
	static double evaluate(std::vector<extended_position>& set)
	{
		double error_sum = 0.0;

		for (extended_position& ep : set)
		{
			boards[0] = ep.board;
			boards[0].generate_eval();

			if constexpr (config::verify_key_phase_eval)
			{
				boards[0].verify_key_phase_eval(ep.side_to_move);
			}

			const eval_t eval = (ep.side_to_move == white) ? alpha_beta<white, true>(0, 0, 0, -eval::mate, eval::mate)
			                                               : -alpha_beta<black, true>(0, 0, 0, -eval::mate, eval::mate);

			const double error = ep.result - sigmoid((double)eval);
			error_sum += (error * error);
		}

		return error_sum / set.size();
	}
#endif

#if tuning
	static void tune_k()
	{
		double best_error = evaluate(extended_positions);
		std::cout << std::format("K = {}, error = {}\n", K, best_error);

		double adjust = 1;
		while (adjust > 0.000000000000001)
		{
			for (const double delta : {adjust, -adjust})
			{
				K += delta;
				double new_error = evaluate(extended_positions);

				if (new_error < best_error)
				{
					best_error = new_error;
					std::cout << std::format("Improved K to {}, error = {} (adjust = {})\n", K, best_error, adjust);
					break;
				}

				K -= delta;
			}

			adjust /= 2;
		}

		std::cout << "Finished tuning K.\n";
	}
#endif

#if tuning
	static void tune_ps_evals()
	{
		std::cout << "Tuning piece-square evals.\n";

		std::mt19937_64 rng{0x0123456789abcdef};

		double test_error_before{};
		double test_error_after{};

		const auto start = util::time_in_ms();

		size_t pass = 1;
		do
		{
			const auto pass_start = util::time_in_ms();

			store_weights();
			store_weights_formatted();

			generate_training_and_test_sets(100'000, rng);

			// Evaluate both sets using the existing weights.
			// We will be tuning using the training set, and keeping the changes.
			const double training_error_before = evaluate(training_set);
			test_error_before = evaluate(test_set);
			double training_error_after = training_error_before;

			for (size_t i = 0; i < eval::ps_evals.size(); ++i)
			{
				for (int8_t delta : {5, -5})
				{
					eval::ps_evals[i] += delta;

					const double new_error = evaluate(training_set);
					if (new_error < training_error_after)
					{
						std::cout << std::format("Improved ps_evals[{:>3}] from {:>4} to {:>4}. New error: {:1.9f}\n",
						    i, int(eval::ps_evals[i] - delta), int(eval::ps_evals[i]), training_error_after);
						training_error_after = new_error;
						break;
					}

					eval::ps_evals[i] -= delta;
				}
			}

			test_error_after = evaluate(test_set);

			const auto now = util::time_in_ms();

			std::stringstream ss;
			ss << std::format("Finished pass {} in {} ms ({} minutes elapsed).\n", pass++, now - pass_start,
			    (now - start + 30'000) / 60'000);
			ss << std::format("Training set error before: {:1.9f}\n", training_error_before);
			ss << std::format("Training set error after:  {:1.9f} ({:+1.9f})\n", training_error_after,
			    training_error_after - training_error_before);
			ss << std::format("Test set error before:     {:1.9f}\n", test_error_before);
			ss << std::format("Test set error after:      {:1.9f} ({:+1.9f})", test_error_after,
			    test_error_after - test_error_before);

			util::log(ss.str());
			std::cout << ss.str() << '\n';
		} while (test_error_after <= test_error_before);

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

		// Make sure we won't exit alpha_beta early.
		scheduled_turn_end = util::time_in_ms() + 1'000'000'000;

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
