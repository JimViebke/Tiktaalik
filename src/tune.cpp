
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "board.hpp"
#include "config.hpp"
#include "game.hpp"
#include "search.hpp"

#if tuning

namespace chess
{
	const std::string base_dir = "D:/Games/chess/";

	const std::string weights_file = base_dir + "engines/tiktaalik weights.txt";
	const std::string formatted_weights_files = base_dir + "engines/tiktaalik weights formatted.txt";
	const std::string games_file = base_dir + "games/CCRL-4040.[1913795]-rated-2500-no-mate.uci";

	// 1: Use the original Texel's Tuning Method.
	// 0: Use a modified tuning method.
	#define texels_method 1

	struct extended_position
	{
		board board;
		color side_to_move;
		float result;
		double error = -2.0;
	};

	std::vector<extended_position> extended_positions;

	#if texels_method
	// For Texel's tuning method, the training set is the full set of
	// extended positions.
	std::vector<extended_position>& training_set = extended_positions;
	#else
	// For the modified tuning method, a training set and test set are
	// generated for each iteration of tuning.
	std::vector<extended_position> training_set;
	std::vector<extended_position> test_set;
	#endif

	using namespace eval;

	// weights is usually available at compile time in evaluation.hpp.
	// During tuning, instead provide it here.
	std::array<int16_t, pse_size + pce_size> eval::weights;

	bool load_weights()
	{
		std::cout << "Loading weights from " << weights_file << '\n';

		std::fstream file(weights_file);

		std::string loaded_weights;
		std::string line;
		while (std::getline(file, line))
		{
			if (line.size() > 0)
			{
				loaded_weights = line;
			}
		}

		std::stringstream ss{loaded_weights};
		eval_t weight = 0;
		for (size_t i = 0; i < weights.size(); ++i)
		{
			if (ss >> weight)
			{
				weights[i] = weight;
			}
			else
			{
				std::cout << std::format("Failed to read weight for weights[{}].\n", i);
				return false;
			}
		}

		if (ss >> weight)
		{
			std::cout << "Found at least one extra weight (?).\n";
			return false;
		}

		std::cout << std::format("Loaded {} weights.\n", eval::weights.size());
		return true;
	}

	static void store_weights()
	{
		std::stringstream ss;

		ss << int(eval::weights[0]);
		for (size_t i = 1; i < weights.size(); ++i)
		{
			ss << ' ' << int(weights[i]);
		}
		ss << '\n';

		std::ofstream file(weights_file, std::ios_base::app);
		file << ss.str();
	}

	static void store_weights_formatted()
	{
		const char left_indent = '\t';
		const std::string left_align = "  ";

		std::stringstream ss;

		ss << "weights = {\n";

		for (size_t i = 0; i < pse_size; ++i)
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
			ss << int(eval::weights[i]) << ',';

			if (i % 8 == 7) ss << '\n';
		}

		ss << left_indent << "};\n";

		std::ofstream ofs(formatted_weights_files, std::ios_base::out);
		ofs << ss.str();
	}

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

		std::mt19937_64 rng{std::mt19937_64::result_type(util::time_in_ms())};

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

			// Get a position from the game by appling a random number of moves between 12 and N inclusive.
			const int position = std::uniform_int_distribution(12, int(moves.size()))(rng);
			for (int i = 0; i < position; ++i)
			{
				apply_move(move{moves[i], boards[0].get_bitboards()});
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

	#if !texels_method
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

	double K = 0.7644;

	static double sigmoid(const double eval)
	{
		const double exp = -K * eval / 400;
		return 1 / (1 + pow(10, exp));
	}

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

	static void tune_weights(const std::string& message, const size_t begin, const size_t size, const eval_t delta)
	{
		{
			std::stringstream ss;
			ss << std::format("Tuning {} ({} weights, at indexes {} through {}) using delta = +/-{}.\n", message, size,
			    begin, begin + size - 1, delta);
	#if texels_method
			ss << "Using Texel's Tuning Method.";
	#else
			ss << "Using modified tuning method.";
	#endif
			util::log(ss.str());
			std::cout << ss.str() << '\n';
		}

		std::mt19937_64 rng{0x0123456789abcdef};
		const auto start = util::time_in_ms();
		size_t pass = 1;

	#if texels_method
		// For Texel's Tuning Method, continue tuning as long as we find
		// changes that reduce the error in the training set.
		const double training_error_before = evaluate(training_set);
		double training_error_after = training_error_before;
		bool improving = true;
	#else
		// For the modified tuning method, continue tuning as long as we
		// find changes that also reduce the error in a test set.
		double test_error_before{};
		double test_error_after{};
	#endif

		do
		{
			const auto pass_start = util::time_in_ms();

			store_weights();
			store_weights_formatted();

	#if texels_method
			improving = false;
	#else
			// Generate a new training and test set for this iteration.
			generate_training_and_test_sets(extended_positions.size() / 2, rng);
			test_error_before = evaluate(test_set);
			const double training_error_before = evaluate(training_set);
			double training_error_after = training_error_before;
	#endif

			for (size_t i = begin; i < begin + size; ++i)
			{
				for (const eval_t d : {delta, -delta})
				{
					weights[i] += d;

					const double new_error = evaluate(training_set);

					if (new_error < training_error_after)
					{
						std::cout << std::format("Improved weights[{:>3}] from {:>4} to {:>4}. New error: {:1.9f}\n", i,
						    int(weights[i] - d), int(weights[i]), new_error);
						training_error_after = new_error;
	#if texels_method
						improving = true;
	#endif
						break;
					}

					weights[i] -= d;
				}
			}

	#if texels_method
	#else
			test_error_after = evaluate(test_set);
	#endif

			const auto now = util::time_in_ms();

			std::stringstream ss;
			ss << std::format("Finished pass {} in {} seconds ({} minutes elapsed).", pass++,
			    (now - pass_start) / 1'000, (now - start) / 60'000);
	#if texels_method
	#else
			ss << '\n';
			ss << std::format("Training set error before: {:1.9f}\n", training_error_before);
			ss << std::format("Training set error after:  {:1.9f} ({:+1.9f})\n", training_error_after,
			    training_error_after - training_error_before);
			ss << std::format("Test set error before:     {:1.9f}\n", test_error_before);
			ss << std::format("Test set error after:      {:1.9f} ({:+1.9f})", test_error_after,
			    test_error_after - test_error_before);
	#endif

			util::log(ss.str());
			std::cout << ss.str() << '\n';
		} while (
	#if texels_method
		    improving
	#else
		    test_error_after < test_error_before
	#endif
		);

	#if texels_method
		std::cout << "Storing final weights.\n";
		store_weights();
		store_weights_formatted();
	#else
		std::cout << "Rolling back to previous best weights.\n";
		load_weights();
	#endif

		std::cout << "Finished tuning.\n";
	}

	static void show_tune_help()
	{
		std::cout << "\ttune help          Display this message.\n";
		std::cout << "\ttune k             Tune the scaling constant K used in sigmoid(eval).\n";
		std::cout << "\ttune all [delta]   Tune all weights using the provided delta.\n";
		std::cout << "\ttune pse [delta]   Tune piece-square evals using the provided delta.\n";
	}

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
			return;
		}

		eval_t delta = 0;
		if (args.size() == 3)
		{
			std::stringstream ss;
			ss << args[2];
			ss >> delta;
		}

		if (delta <= 0)
		{
			std::cout << std::format("Invalid delta: {}\n", int(delta));
			return;
		}

		if (args[1] == "all")
		{
			load_games();
			tune_weights("all weights", 0, weights.size(), delta);
		}
		else if (args[1] == "pse")
		{
			load_games();
			tune_weights("piece-square evals", pse_start, pse_size, delta);
		}
		else
		{
			std::cout << "Tune command unrecognized/invalid.\n";
			show_tune_help();
		}
	}
}

#endif // tuning
