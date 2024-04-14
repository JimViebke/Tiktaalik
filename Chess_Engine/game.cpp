/* 2021 December 16 */

#include "game.hpp"

namespace chess
{
	void Game::worker_thread()
	{
		std::cout << "worker thread started\n";

		while (1)
		{
			const std::lock_guard<decltype(game_mutex)> lock(game_mutex);

			// visit all ply1 nodes
			for (Node& child_node : root.children)
			{
				// if a ply1 node does not have ply2 children, generate its ply2 children
				if (child_node.children.empty())
				{
					child_node.generate_child_boards();
					break;
				}
			}
		}
	}
}
