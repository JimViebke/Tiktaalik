#pragma once

#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <variant>

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>

#include "move.hpp"
#include "notation.hpp"
#include "perft.hpp"
#include "search.hpp"
#include "uci.hpp"
#include "util/util.hpp"

namespace chess
{
	namespace detail
	{
		constexpr size_t window_width = 1500;
		constexpr size_t window_height = 900;

		constexpr size_t board_x = 50;
		constexpr size_t board_y = 100;

		const sf::Color background = { 30, 30, 30 };
		const sf::Color text_color = sf::Color::White;
		const sf::Color brown = { 153, 76, 0 }; // brown
		const sf::Color cream = { 248, 194, 150 }; // cream

		constexpr size_t tile_size_px = 90;
		constexpr size_t board_size_px = tile_size_px * 8;

		constexpr size_t default_font_size = 20;

		constexpr size_t right_overlay_x = board_size_px + board_x * 2;
		constexpr size_t right_overlay_y = board_y / 2;
		constexpr size_t right_overlay_font_size = default_font_size;

		constexpr size_t legal_marker_radius_px = tile_size_px / 5;

		constexpr size_t piece_resolution_px = 150; // use 150x150, the native piece resolution
	}

	const std::string start_pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	const std::string kiwipete = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 0";
	const std::string position_5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
	const std::string sicilian_defense = "rnbqkbnr/pp2pppp/3p4/2p5/3PP3/5N2/PPP2PPP/RNBQKB1R b KQkq d3 0 3";

	const std::string colle_v_grau_M3 = "1k5r/pP3ppp/3p2b1/1BN1n3/1Q2P3/P1B5/KP3P1P/7q w - - 1 0";

	const std::string yates_v_nimzowitsch_M4 = "8/k2r4/p7/2b1Bp2/P3p3/qp4R1/4QP2/1K6 b - - 0 1";
	const std::string journoud_v_riviere_M4 = "r1bk3r/pppq1ppp/5n2/4N1N1/2Bp4/Bn6/P4PPP/4R1K1 w - - 1 0";
	const std::string keres_v_salamanca_M4 = "7R/r1p1q1pp/3k4/1p1n1Q2/3N4/8/1PP2PPP/2B3K1 w - - 1 0";
	const std::string meek_v_morphy_M4 = "Q7/p1p1q1pk/3p2rp/4n3/3bP3/7b/PP3PPK/R1B2R2 b - - 0 1";
	const std::string topalow_v_kasparov_M4 = "4k2r/1R3R2/p3p1pp/4b3/1BnNr3/8/P1P5/5K2 w - - 1 0";
	const std::string korchnoi_v_peterson_M4 = "r2r1n2/pp2bk2/2p1p2p/3q4/3PN1QP/2P3R1/P4PP1/5RK1 w - - 0 1";
	const std::string agdestein_v_al_qudaimi_M4 = "3r1r2/1pp2p1k/p5pp/4P3/2nP3R/2P3QP/P1B1q1P1/5RK1 w - - 1 0";
	const std::string giese_v_alekhine_M4 = "2k4r/ppp2p2/2b2B2/7p/6pP/2P1q1bP/PP3N2/R4QK1 b - - 0 1";

	const std::string short_mate_test_M2 = "6k1/P6R/8/8/8/8/8/4K3 w - - 0 0";
	const std::string two_rooks_endgame_w = "4k3/8/8/8/8/8/8/R3K2R w - - 0 0";
	const std::string one_rook_endgame_w = "4k3/8/8/8/8/8/8/R3K3 w - - 0 0";
	const std::string two_rooks_endgame_b_M2 = "r3k2r/8/8/8/8/8/8/4K3 b - - 0 0";
	const std::string white_in_check = "4k3/8/8/8/8/4K2r/8/8 w - - 0 0";

	const std::string start_fen = start_pos;

	color_t load_fen(const std::string& fen, position& _position);

	// Forward-declaring.
	void send_command(const std::string& command);

	class Game
	{
	public:
		Game() : color_to_move{ load_fen(start_fen, positions[0]) }
		{
			update_info_for_new_root_position();

			searching.store(false);
			std::thread([this] { worker_thread(); }).detach();
		}

		void send_info(const eval_t eval);
		void apply_moves(const std::vector<std::string>& args, size_t move_idx);
		void process_position_command(const std::vector<std::string>& args);
		void process_go_command(const std::vector<std::string>& args);
		void process_uci_commands();

	private:
		bool mouse_on_board() const
		{
			return
				mouse_x > int(detail::board_x) && mouse_x < int(detail::board_x + detail::board_size_px) &&
				mouse_y > int(detail::board_y) && mouse_y < int(detail::board_y + detail::board_size_px);
		}

		void on_click()
		{
			if (!mouse_on_board()) return;

			const position& root_position = positions[0];

			if (root_position.is_empty(mouse_square_y, mouse_square_x))
			{
				std::cout << "No piece to drag\n";
				return;
			}

			if (!root_position.piece_at(mouse_square_y, mouse_square_x).is_color(color_to_move))
			{
				std::cout << "Not this color's turn\n";
				return;
			}

			// Only allow dragging if we can find a legal move starting from the clicked square.
			for (const board& move : moves)
			{
				const std::string legal_move = move.move_to_string();
				const file legal_move_x = char_to_file(legal_move[0]);
				const rank legal_move_y = char_to_rank(legal_move[1]);

				if (legal_move_x == mouse_square_x && legal_move_y == mouse_square_y)
				{
					dragging = true;
					dragging_piece = root_position.piece_at(mouse_square_y, mouse_square_x);
					dragging_from_x = mouse_square_x;
					dragging_from_y = mouse_square_y;
					std::cout << "drag...\n";
					return;
				}
			}

			std::cout << "This piece can't move\n";
		}

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

		bool get_ply_1_board_idx(size_t& result, const board& board)
		{
			constexpr size_t begin_idx = first_child_index(0);

			for (size_t idx = begin_idx; idx != begin_idx + moves.size(); ++idx)
			{
				if (same_move(boards[idx], board))
				{
					result = idx;
					return true;
				}
			}

			return false;
		}

		// Index must be a ply-1 child of the root position.
		// The calling thread must own the game mutex.
		void apply_move(const size_t index)
		{
			// Update root color, position, and board.
			color_to_move = other_color(color_to_move);
			positions[0] = positions[index];
			boards[0] = boards[index];

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
			bool found_move = false;
			for (size_t i = first_child_index(0); i < first_child_index(0) + moves.size(); ++i)
			{
				if (boards[i].move_to_string() == move)
				{
					apply_move(i);
					found_move = true;
					break;
				}
			}

			if (!found_move)
			{
				std::stringstream ss;
				ss << "Illegal move found in position command: [" << move << ']';
				util::log(ss.str());
			}
		}

		void on_release()
		{
			if (!dragging) return;

			dragging = false;
		}

		void handle_events()
		{
			using namespace detail;

			sf::Event event;
			while (window->pollEvent(event))
			{
				// supress warning "18 enumeration values not handled in switch"
			#pragma clang diagnostic push
			#pragma clang diagnostic ignored "-Wswitch"
				switch (event.type)
				{
					case sf::Event::MouseMoved:
						mouse_x = event.mouseMove.x;
						mouse_y = event.mouseMove.y;
						break;

					case sf::Event::MouseButtonPressed:
						if (event.mouseButton.button == sf::Mouse::Button::Left)
							on_click();
						break;

					case sf::Event::MouseButtonReleased:
						if (event.mouseButton.button == sf::Mouse::Button::Left)
							on_release();
						break;

					case sf::Event::MouseWheelScrolled:
						right_overlay_scroll += event.mouseWheelScroll.delta * detail::right_overlay_font_size;
						break;

					case sf::Event::KeyPressed:
						if (event.key.code == sf::Keyboard::Key::Space)
							// play_move();
							break;

					case sf::Event::Closed:
						window->close();
						break;
				}
			#pragma clang diagnostic pop
			}
		}

		void draw_box(const size_t x, const size_t y, const size_t width, const size_t height, const sf::Color color,
					  bool outline = false, const sf::Color outline_color = sf::Color::Black, const float outline_thickness = -2.f)
		{
			sf::RectangleShape box{ { (float)width, (float)height } };
			box.setPosition({ (float)x, (float)y });
			box.setFillColor(color);

			if (outline)
			{
				box.setOutlineColor(outline_color);
				box.setOutlineThickness(outline_thickness);
			}

			window->draw(box);
		}

		void generate_right_overlay()
		{
			std::stringstream ss;

			if (color_to_move == white)
				ss << "White to move\n\n";
			else
				ss << "Black to move\n\n";

			ss << n_of_evals << " positions evaluated in " << engine_time << " ms, depth " << engine_depth << '\n';

			// List legal moves if any exist.
			if (moves.size() > 0)
			{
				if (pv_lengths[0] > 0)
				{
					size_t idx = 0;
					if (!get_ply_1_board_idx(idx, pv_moves[0][0]))
					{
						// should never happen
						std::cout << "generate_right_overlay() can't find pv_moves[0][0]: " << pv_moves[0][0].move_to_string() << std::endl;
					}

					ss << "Best move: ";
					move_to_notation(ss, 0, idx, other_color(color_to_move));
					ss << '\n';

					// Create a second stringstream here so we can send the same information to the console and GUI.
					std::stringstream ss2;
					ss2 << std::fixed << std::setprecision(1) << float(pv_moves[0][0].get_eval()) / 100 << '\t';
					for (size_t i = 0; i < pv_lengths[0]; ++i)
					{
						ss2 << pv_moves[0][i].move_to_string() << ' ';
					}
					ss2 << '\n';

					std::cout << ss2.str();
					ss << ss2.str() << '\n';
				}

				ss << moves.size() << " moves:\n";
				const size_t begin_idx = first_child_index(0);
				for (size_t idx = begin_idx; idx < begin_idx + moves.size(); ++idx)
				{
					move_to_notation(ss, 0, idx, other_color(color_to_move));
					ss << '\n';
				}
			}

			// Replace the old stringstream.
			std::lock_guard<decltype(right_overlay_mutex)> lock(right_overlay_mutex);
			right_overlay_ss = std::move(ss);
		}

		void tick()
		{
			/*
			This function contains things that need to happen once per tick, and are not directly related to
			event handling or rendering.
			*/

			++tick_counter;

			if (mouse_on_board())
			{
				mouse_square_x = (mouse_x - detail::board_x) / detail::tile_size_px;
				mouse_square_y = (mouse_y - detail::board_y) / detail::tile_size_px;
			}
		}

		void render_piece(const piece& piece, const float x_pos, const float y_pos)
		{
			if (piece.is_white())
			{
				if (piece.is_rook()) { wr.setPosition({ x_pos, y_pos }); window->draw(wr); }
				else if (piece.is_knight()) { wn.setPosition({ x_pos, y_pos }); window->draw(wn); }
				else if (piece.is_bishop()) { wb.setPosition({ x_pos, y_pos }); window->draw(wb); }
				else if (piece.is_queen()) { wq.setPosition({ x_pos, y_pos }); window->draw(wq); }
				else if (piece.is_king()) { wk.setPosition({ x_pos, y_pos }); window->draw(wk); }
				else if (piece.is_pawn()) { wp.setPosition({ x_pos, y_pos }); window->draw(wp); }
			}
			else
			{
				if (piece.is_rook()) { br.setPosition({ x_pos, y_pos }); window->draw(br); }
				else if (piece.is_knight()) { bn.setPosition({ x_pos, y_pos }); window->draw(bn); }
				else if (piece.is_bishop()) { bb.setPosition({ x_pos, y_pos }); window->draw(bb); }
				else if (piece.is_queen()) { bq.setPosition({ x_pos, y_pos }); window->draw(bq); }
				else if (piece.is_king()) { bk.setPosition({ x_pos, y_pos }); window->draw(bk); }
				else if (piece.is_pawn()) { bp.setPosition({ x_pos, y_pos }); window->draw(bp); }
			}
		}

		void render_game_board()
		{
			using namespace detail;

			// the board background
			draw_box(board_x, board_y, board_size_px, board_size_px, cream);

			// draw 32 tiles
			for (size_t i = 0; i < 4; ++i)
				for (size_t j = 0; j < 4; ++j)
					draw_box(
						board_x + tile_size_px + i * 2 * tile_size_px,
						board_y + j * 2 * tile_size_px,
						tile_size_px, tile_size_px, brown);
			for (size_t i = 0; i < 4; ++i)
				for (size_t j = 0; j < 4; ++j)
					draw_box(
						board_x + i * 2 * tile_size_px,
						board_y + tile_size_px + j * 2 * tile_size_px,
						tile_size_px, tile_size_px, brown);

			const position& root_position = positions[0];

			// As part of normalizing x/y vs rank/file, make this block unaware of rank/file, just x/y.

			for (int rank = 7; rank >= 0; --rank)
			{
				for (int file = 0; file < 8; ++file)
				{
					const piece piece = root_position.piece_at(rank, file);
					if (piece.is_empty()) continue;

					// If the current piece is being dragged, don't draw it in the original position
					if (dragging && file == int(dragging_from_x) && rank == int(dragging_from_y))
						continue;

					const float x_pos = board_x + float(file * tile_size_px);
					const float y_pos = board_y + float(rank * tile_size_px);

					render_piece(piece, x_pos, y_pos);
				}
			}

			// Draw a marker for each square the selected piece can move to.
			if (dragging)
			{
				for (const board& move : moves)
				{
					const std::string move_str = move.move_to_string();
					const auto move_x = char_to_file(move_str[0]);
					const auto move_y = char_to_rank(move_str[1]);

					if (dragging_from_x == move_x &&
						dragging_from_y == move_y)
					{
						const auto move_to_x = char_to_file(move_str[2]);
						const auto move_to_y = char_to_rank(move_str[3]);

						// Add half of a tile's size, then remove the marker's radius
						constexpr float adjust = (detail::tile_size_px / 2) - legal_marker_radius_px;

						legal_marker.setPosition(
							float(board_x + detail::tile_size_px * move_to_x.value() + adjust),
							float(board_y + detail::tile_size_px * move_to_y.value() + adjust));

						window->draw(legal_marker);
					}
				}

				render_piece(dragging_piece, float(mouse_x - tile_size_px / 2), float(mouse_y - tile_size_px / 2));
			}
		}
		void render_overlay()
		{
			std::stringstream debug;
			debug << tick_counter % 60 << '\n';
			debug << mouse_x << ", " << mouse_y << '\n';
			if (mouse_on_board())
			{
				debug << mouse_square_x << ", " << mouse_square_y << '\n';
			}
			overlay.setString(debug.str());
			window->draw(overlay);
		}
		void render_right_overlay()
		{
			{
				std::lock_guard<decltype(right_overlay_mutex)> lock(right_overlay_mutex);
				right_overlay.setString(right_overlay_ss.str());
			}

			// update the position to handle scroll events
			right_overlay.setPosition({ detail::right_overlay_x, detail::right_overlay_y + right_overlay_scroll });
			window->draw(right_overlay);
		}

		void render()
		{
			window->clear(detail::background);

			render_game_board();
			render_overlay();
			render_right_overlay();
		}

		template<color_t color_to_move>
		eval_t search(const size_t end_idx, depth_t depth);

		void worker_thread()
		{
			// Sleep until the main thread sets searching to true.
			searching.wait(false);
			std::unique_lock<decltype(game_mutex)> lock(game_mutex); // constructs and locks

			std::string best_move;

			while (1)
			{
				if (!searching)
				{
					// Stop searching and release the mutex until the main thread tells us to resume.
					lock.unlock();
					searching.wait(false);
					lock.lock();

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
					// We stopped searching because we ran out of planned time.
					// Send the best move if we have one.
					if (best_move.size() != 0)
					{
						// En Croissant workaround: send the best move as a pv.
						{
							std::stringstream ss;
							ss << "info pv " << best_move;
							send_command(ss.str());
						}

						std::stringstream ss;
						ss << "bestmove ";
						ss << best_move;
						send_command(ss.str());

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

		void run()
		{
			std::thread([this] { worker_thread(); }).detach();

			while (window->isOpen())
			{
				// handle all updates...
				{
					handle_events();
					tick();
					render();
				}
				// ...then release the mutex before rendering and sleeping

				window->display();
			}
		}

	private:
		// Used to regulate reading and modification of the game state between the GUI and worker threads.
		std::mutex game_mutex;

		size_t tick_counter = 0;

		int32_t mouse_x = 0, mouse_y = 0;
		size_t mouse_square_x = 0, mouse_square_y = 0;

		bool dragging{ false };
		piece dragging_piece{}; // default-constructs to "empty"
		size_t dragging_from_x = 0, dragging_from_y = 0;

		std::unique_ptr<sf::RenderWindow> window;
		sf::Font arial;
		sf::Text overlay; // starts in the very top left
		sf::Text right_overlay; // main text overlay
		std::stringstream right_overlay_ss;
		float right_overlay_scroll = 0.f;
		std::mutex right_overlay_mutex;

		color_t color_to_move;

		depth_t engine_depth = 0;
		size_t n_of_evals = 0;
		util::timepoint engine_time = 0;

		std::vector<board> moves;

		sf::Texture wk_texture;
		sf::Texture wq_texture;
		sf::Texture wb_texture;
		sf::Texture wn_texture;
		sf::Texture wr_texture;
		sf::Texture wp_texture;

		sf::Texture bk_texture;
		sf::Texture bq_texture;
		sf::Texture bb_texture;
		sf::Texture bn_texture;
		sf::Texture br_texture;
		sf::Texture bp_texture;

		sf::Sprite wk;
		sf::Sprite wq;
		sf::Sprite wb;
		sf::Sprite wn;
		sf::Sprite wr;
		sf::Sprite wp;

		sf::Sprite bk;
		sf::Sprite bq;
		sf::Sprite bb;
		sf::Sprite bn;
		sf::Sprite br;
		sf::Sprite bp;

		sf::CircleShape legal_marker;
	};
}
