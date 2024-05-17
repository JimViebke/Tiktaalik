#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <variant>

#include "move.hpp"
#include "node.hpp"
#include "notation.hpp"
#include "search.hpp"
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

	const std::string short_mate_test = "6k1/P6R/8/8/8/8/8/4K3 w - - 0 0";
	const std::string two_rooks_endgame_w = "4k3/8/8/8/8/8/8/R3K2R w - - 0 0";
	const std::string one_rook_endgame_w = "4k3/8/8/8/8/8/8/R3K3 w - - 0 0";
	const std::string two_rooks_endgame_b = "r3k2r/8/8/8/8/8/8/4K3 b - - 0 0";

	const std::string start_fen = start_pos;

	root_v load_fen(const std::string& fen, position& _position);

	class Game
	{
	public:
		Game() : root{ load_fen(start_fen, positions[0]) }
		{
			sf::ContextSettings settings;
			settings.antialiasingLevel = 8;

			window = std::make_unique<sf::RenderWindow>(
				sf::VideoMode((uint32_t)detail::window_width, (uint32_t)detail::window_height),
				"Chess engine",
				sf::Style::Close,
				settings);

			window->setFramerateLimit(60);

			const std::string ARIAL_LOCATION = "C:/Windows/Fonts/Arial.ttf";
			if (!arial.loadFromFile(ARIAL_LOCATION))
			{
				std::cout << "Could not load " << ARIAL_LOCATION << '\n';
				abort();
			}

			if (auto white_root = std::get_if<node<white>>(&root))
			{
				white_root->generate_child_boards(positions[0]);
				best_move = white_root->children.data(); // can be null
			}
			else if (auto black_root = std::get_if<node<black>>(&root))
			{
				black_root->generate_child_boards(positions[0]);
				best_move = black_root->children.data(); // can be null
			}
			else
			{
				std::cout << "Failed to get root node in Game()\n";
			}

			legal_marker.setRadius(detail::legal_marker_radius_px);
			legal_marker.setPointCount(15);
			legal_marker.setFillColor({ 50, 50, 50, 255 / 2 });

			overlay.setFont(arial);
			overlay.setCharacterSize(detail::default_font_size);
			overlay.setFillColor(detail::text_color);

			right_overlay.setFont(arial);
			right_overlay.setCharacterSize(detail::right_overlay_font_size);
			right_overlay.setFillColor(detail::text_color);
			right_overlay.setPosition({ detail::board_size_px + detail::board_x * 2, detail::board_y / 2 });

			if (!wk_texture.loadFromFile("../Chess_Engine/res/wk.png"))
				std::cout << "wk failed to load";
			if (!wq_texture.loadFromFile("../Chess_Engine/res/wq.png"))
				std::cout << "wq failed to load";
			if (!wb_texture.loadFromFile("../Chess_Engine/res/wb.png"))
				std::cout << "wb failed to load";
			if (!wn_texture.loadFromFile("../Chess_Engine/res/wn.png"))
				std::cout << "wn failed to load";
			if (!wr_texture.loadFromFile("../Chess_Engine/res/wr.png"))
				std::cout << "wr failed to load";
			if (!wp_texture.loadFromFile("../Chess_Engine/res/wp.png"))
				std::cout << "wp failed to load";

			if (!bk_texture.loadFromFile("../Chess_Engine/res/bk.png"))
				std::cout << "bk failed to load";
			if (!bq_texture.loadFromFile("../Chess_Engine/res/bq.png"))
				std::cout << "bq failed to load";
			if (!bb_texture.loadFromFile("../Chess_Engine/res/bb.png"))
				std::cout << "bb failed to load";
			if (!bn_texture.loadFromFile("../Chess_Engine/res/bn.png"))
				std::cout << "bn failed to load";
			if (!br_texture.loadFromFile("../Chess_Engine/res/br.png"))
				std::cout << "br failed to load";
			if (!bp_texture.loadFromFile("../Chess_Engine/res/bp.png"))
				std::cout << "bp failed to load";

			wk.setTexture(wk_texture);
			wq.setTexture(wq_texture);
			wb.setTexture(wb_texture);
			wn.setTexture(wn_texture);
			wr.setTexture(wr_texture);
			wp.setTexture(wp_texture);

			bk.setTexture(bk_texture);
			bq.setTexture(bq_texture);
			bb.setTexture(bb_texture);
			bn.setTexture(bn_texture);
			br.setTexture(br_texture);
			bp.setTexture(bp_texture);

			// scale
			constexpr float scaling = (float)detail::tile_size_px / (float)detail::piece_resolution_px;

			wk.setScale(scaling, scaling);
			wq.setScale(scaling, scaling);
			wb.setScale(scaling, scaling);
			wn.setScale(scaling, scaling);
			wr.setScale(scaling, scaling);
			wp.setScale(scaling, scaling);

			bk.setScale(scaling, scaling);
			bq.setScale(scaling, scaling);
			bb.setScale(scaling, scaling);
			bn.setScale(scaling, scaling);
			br.setScale(scaling, scaling);
			bp.setScale(scaling, scaling);
		}

	private:
		bool mouse_on_board() const
		{
			return
				mouse_x > int(detail::board_x) && mouse_x < int(detail::board_x + detail::board_size_px) &&
				mouse_y > int(detail::board_y) && mouse_y < int(detail::board_y + detail::board_size_px);
		}

		template<color_t color_to_move>
		void on_click(const node<color_to_move>& root_node)
		{
			const position& root_position = positions[0];

			/*
			Typically, x increases to the right, and y increases downward.
			In chess, rank increases upward, and file increases to the right.
			This requires some translation. */
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

			// Only allow dragging if we can find a legal move starting from the clicked square
			for (const auto& child_node : root_node.children)
			{
				const std::string legal_move = child_node.get_board().move_to_string();
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

		void on_click()
		{
			if (!mouse_on_board()) return;

			if (auto white_root = std::get_if<node<white>>(&root))
				on_click(*white_root);
			else if (auto black_root = std::get_if<node<black>>(&root))
				on_click(*black_root);
			else
				std::cout << "Failed to get root node in on_click()\n";
		}

		template<color_t c1, color_t c2>
			requires (c1 != c2)
		void play_move(node<c1>& root_node, node<c2>& child_node)
		{
			// update the root position
			make_move<c2>(positions[0], positions[0], child_node.get_board());
			child_node.index = 0;

			// Move a ply-1 child node to become the root node.
			// This preserves the relevant subset of the move graph.
			// This must be done in two steps, because any form of `root = move(child)`
			// will clobber the move graph before the move takes place.
			auto new_root = std::move(child_node);
			root = std::move(new_root);

			// reset best_move
			best_move = decltype(&root_node){}; // change type and set to nullptr

			// Decrement whatever the current depth is because we're advancing down the tree by one node.
			if (engine_depth > 0)
				--engine_depth;
		}

		template<typename node_t>
		void on_release(node_t& root_node)
		{
			// Check that [from_x, from_y, to_x, to_y] is a legal move
			for (auto& child_node : root_node.children)
			{
				const std::string legal_move = child_node.get_board().move_to_string();
				const auto legal_start_x = char_to_file(legal_move[0]);
				const auto legal_start_y = char_to_rank(legal_move[1]);
				const auto legal_end_x = char_to_file(legal_move[2]);
				const auto legal_end_y = char_to_rank(legal_move[3]);

				if (dragging_from_x == legal_start_x &&
					dragging_from_y == legal_start_y &&
					mouse_square_x == legal_end_x &&
					mouse_square_y == legal_end_y)
				{
					play_move(root_node, child_node);
					return;
				}
			}

			std::cout << "(illegal move)\n";
		}

		void on_release()
		{
			if (!dragging) return;

			dragging = false;

			if (!mouse_on_board())
			{
				std::cout << "(tried to drag a piece off of the board)\n";
				return;
			}

			std::cout << "...and drop\n";

			// Check that the x or y has changed, otherwise this was a non-move
			if (dragging_from_x == mouse_square_x &&
				dragging_from_y == mouse_square_y)
			{
				std::cout << "Dragged a piece without moving it\n";
				return;
			}
			else
			{
				std::cout << "Dragging from "
					<< dragging_from_x << ',' << dragging_from_y << " to "
					<< mouse_square_x << ',' << mouse_square_y << '\n';
			}

			if (!mouse_on_board()) return;

			if (auto white_root = std::get_if<node<white>>(&root))
				on_release(*white_root);
			else if (auto black_root = std::get_if<node<black>>(&root))
				on_release(*black_root);
			else
				std::cout << "Failed to get root node in on_release()\n";
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
						if (event.key.code == sf::Keyboard::Key::Space &&
							engine_depth == config::engine_target_depth)
						{
							if (auto white_root = std::get_if<node<white>>(&root))
							{
								if (auto move = *std::get_if<node<black>*>(&best_move))
									play_move(*white_root, *move);
							}
							else if (auto black_root = std::get_if<node<black>>(&root))
							{
								if (auto move = *std::get_if<node<white>*>(&best_move))
									play_move(*black_root, *move);
							}
						}
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

		void render_game_board(const auto& root_node)
		{
			using namespace detail;

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

			if (dragging)
			{
				for (const auto& child_node : root_node.children)
				{
					const std::string move = child_node.get_board().move_to_string();
					const auto move_x = char_to_file(move[0]);
					const auto move_y = char_to_rank(move[1]);

					if (dragging_from_x == move_x &&
						dragging_from_y == move_y)
					{
						const auto move_to_x = char_to_file(move[2]);
						const auto move_to_y = char_to_rank(move[3]);

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

			if (auto white_root = std::get_if<node<white>>(&root))
				render_game_board(*white_root);
			else if (auto black_root = std::get_if<node<black>>(&root))
				render_game_board(*black_root);
			else
				std::cout << "Failed to get root node in render_game_board()\n";
		}

		template<typename node_t>
		void render_right_overlay(const node_t& root_node)
		{
			std::stringstream ss;

			if constexpr (root_node.white_to_move())
				ss << "White to move\n\n";
			else
				ss << "Black to move\n\n";

			ss << n_of_evals << " positions evaluated in " << engine_time << " ms, depth " << engine_depth << '\n';

			if (auto child_node = *std::get_if<typename node_t::other_node_t*>(&best_move))
			{
				ss << "Best move: ";
				move_to_notation(ss, positions[0], *child_node);
				ss << '\n' << std::fixed << std::setprecision(1) << float(boards[child_node->index].get_eval()) / 100 << '\n';
			}
			ss << '\n';

			ss << root_node.children.size() << " moves:\n";
			for (const auto& child_node : root_node.children)
			{
				move_to_notation(ss, positions[0], child_node);
				ss << '\n';
			}

			right_overlay.setString(ss.str());
			right_overlay.setPosition({ detail::right_overlay_x, detail::right_overlay_y + right_overlay_scroll });
			window->draw(right_overlay);
		}

		void render_right_overlay()
		{
			if (auto white_root = std::get_if<node<white>>(&root))
				render_right_overlay(*white_root);
			else if (auto black_root = std::get_if<node<black>>(&root))
				render_right_overlay(*black_root);
			else
				std::cout << "Failed to get root node in render_right_overlay()\n";
		}

		void render()
		{
			window->clear(detail::background);

			render_game_board();

			std::stringstream debug;
			debug << tick_counter % 60 << '\n';
			debug << mouse_x << ", " << mouse_y << '\n';
			if (mouse_on_board())
			{
				debug << mouse_square_x << ", " << mouse_square_y << '\n';
			}
			overlay.setString(debug.str());
			window->draw(overlay);

			render_right_overlay();
		}

		template<typename node_t>
		void worker_thread(node_t& root_node)
		{
			std::cout << "Engine evaluating as " << (root_node.white_to_move() ? "white\n" : "black\n");

			const auto start_time = util::time_in_ms();

			n_of_evals = 0;
			best_move = search(root_node, engine_depth, n_of_evals);

			engine_time = util::time_in_ms() - start_time;

			std::cout << "depth " << engine_depth << ", " << n_of_evals << " evals, " << engine_time << " ms\n";
			std::cout << "\ttt hit: " << detail::tt_hit << " tt miss: " << detail::tt_miss << '\n';
			std::cout << "\toccupied: " << detail::tt.occupied_entries << " insertions: " << detail::tt.insertions << " updates: " << detail::tt.updates << '\n';

			detail::tt_hit = 0;
			detail::tt_miss = 0;
		}

		void worker_thread()
		{
			std::cout << "Worker thread started with depth " << engine_depth << '\n';

			while (1)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));

				const std::lock_guard<decltype(game_mutex)> lock(game_mutex);

				if (engine_depth < config::engine_target_depth)
				{
					++engine_depth;

					if (auto white_root = std::get_if<node<white>>(&root))
						worker_thread(*white_root);
					else if (auto black_root = std::get_if<node<black>>(&root))
						worker_thread(*black_root);
					else
						std::cout << "Failed to get root node in worker_thread()\n";
				}
			}
		}

		void menu(auto& root_node, const std::string& command, const depth_t depth)
		{
			if (command == "perft")
				root_node.perft(positions[0], depth);
			else if (command == "divide")
				root_node.divide(positions[0], depth);
			else
				std::cout << "Unknown command: " << command << '\n';
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

					if (auto white_root = std::get_if<node<white>>(&root))
						menu(*white_root, command, depth);
					else if (auto black_root = std::get_if<node<black>>(&root))
						menu(*black_root, command, depth);
					else
						std::cout << "Failed to get root node in menu()\n";
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
					const std::lock_guard<decltype(game_mutex)> lock(game_mutex);

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
		float right_overlay_scroll = 0.f;

		root_v root; // move graph rooted on the current position
		best_move_v best_move{}; // pointer to a ply-1 child

		depth_t engine_depth = 0;
		size_t n_of_evals = 0;
		util::timepoint engine_time = 0;

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
