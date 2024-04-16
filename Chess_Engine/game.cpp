/* 2021 December 16 */

#include "game.hpp"
#include "search.hpp"

namespace chess
{
	void Game::worker_thread()
	{
		std::cout << "worker thread started\n";

		while (1)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

			const std::lock_guard<decltype(game_mutex)> lock(game_mutex);

			best_move = alpha_beta(root, 5, root.board.white_to_move());
		}
	}
}
