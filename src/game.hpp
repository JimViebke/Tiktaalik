#pragma once

#include <format>
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
	extern const std::string start_pos;

	void send_command(const std::string& command); // Forward-declare.
#if tuning
	bool load_weights(); // Forward-declare.
#endif

	class game
	{
	public:
		game();
		void process_uci_commands();

	private:
		void send_info(const eval_t eval);
		void apply_moves(const std::vector<std::string>& args, size_t move_idx);
		void process_position_command(const std::vector<std::string>& args);
		void process_go_command(const std::vector<std::string>& args);

		void generate_child_boards_for_root();

		// Index must be a ply-1 child of the root position.
		// The caller must own the game mutex.
		void apply_move(const size_t index);

		// `move` is either in the form "e4f5", or "c7c8q" in the case of promotion.
		// The caller must own the game mutex.
		void apply_move(const std::string& move);

		void send_move(const std::string& move);

		template <color color_to_move>
		eval_t search(const size_t end_idx, const depth_t depth);

		void worker_thread();

#if tuning
		void load_games();
		void tune(const std::vector<std::string>& args);
#endif

		std::mutex game_mutex;

		color color_to_move;

		depth_t engine_depth = 0;
		util::timepoint engine_start_time = 0;
		util::timepoint engine_time = 0;

		size_t n_legal_moves = 0;
	};
}
