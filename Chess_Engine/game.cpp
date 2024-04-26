/* 2021 December 16 */

#include "game.hpp"
#include "search.hpp"

namespace chess
{
	constexpr size_t engine_target_depth = 6;
	constexpr size_t step = 1;

	void Game::worker_thread()
	{
		std::cout << "Worker thread started with depth " << engine_depth << '\n';

		while (1)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

			const std::lock_guard<decltype(game_mutex)> lock(game_mutex);

			if (engine_depth < engine_target_depth)
			{
				engine_depth += step;

				std::cout << "Engine evaluating as " << (root.board.white_to_move() ? "white\n" : "black\n");

				const auto start_time = util::time_in_ms();

				parent_of_best_move = &root; // currently, this is always the case

				n_of_evals = 0;
				if (root.board.white_to_move())
					result_of_best_move = alpha_beta<true>(root, engine_depth, n_of_evals);
				else
					result_of_best_move = alpha_beta<false>(root, engine_depth, n_of_evals);

				engine_time = util::time_in_ms() - start_time;

				std::cout << "depth " << engine_depth << ", " << n_of_evals << " evals, " << engine_time << " ms\n";
			}

		}
	}

	void Game::menu()
	{
		const std::lock_guard<decltype(game_mutex)> lock(game_mutex);
		window->setVisible(false);

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
				size_t depth = 0;
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

				root.children.clear(); // suppress a debug printout (todo: remove this)

				if (command == "perft")
					root.perft(depth);
				else
					root.divide(depth);
			}
			else if (command == "quit" || command == "q")
			{
				return;
			}
		}
	}
}
