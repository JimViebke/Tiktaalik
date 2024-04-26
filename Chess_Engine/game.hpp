#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <iostream>
#include <string>
#include <sstream>
#include <mutex>
#include <thread>

#include "node.hpp"
#include "notation.hpp"

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

		constexpr size_t legal_marker_radius_px = tile_size_px / 5;

		constexpr size_t piece_resolution_px = 150; // use 150x150, the native piece resolution
	}

	class Game
	{
	public:
		Game() : root{ Board{ layouts::start_board, white } }
		{
			sf::ContextSettings settings;
			settings.antialiasingLevel = 8;

			const std::lock_guard<decltype(game_mutex)> lock(game_mutex);
			std::thread([this] { worker_thread(); }).detach();

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

			root.generate_child_boards();

			legal_marker.setRadius(detail::legal_marker_radius_px);
			legal_marker.setPointCount(15);
			legal_marker.setFillColor({ 50, 50, 50, 255 / 2 });

			overlay.setFont(arial);
			overlay.setCharacterSize(20);
			overlay.setFillColor(detail::text_color);

			right_overlay.setFont(arial);
			right_overlay.setCharacterSize(20);
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
				mouse_x > detail::board_x && mouse_x < detail::board_x + detail::board_size_px &&
				mouse_y > detail::board_y && mouse_y < detail::board_y + detail::board_size_px;
		}

		void on_click()
		{
			if (!mouse_on_board()) return;

			/*
			Typically, x increases to the right, and y increases downward.
			In chess, rank increases upward, and file increases to the right.
			This requires some translation. */
			if (root.board.is_empty(mouse_square_y, mouse_square_x))
			{
				std::cout << "No piece to drag\n";
				return;
			}

			if (!root.board.piece_at(mouse_square_y, mouse_square_x).is_color(root.board.get_color_to_move()))
			{
				std::cout << "Not this color's turn\n";
				return;
			}

			// Only allow dragging if we can find a legal move starting from the clicked square
			for (const Node& child_node : root.children)
			{
				const std::string legal_move = child_node.board.move_to_string();
				const file legal_move_x = char_to_file(legal_move[0]);
				const rank legal_move_y = char_to_rank(legal_move[1]);

				if (legal_move_x == mouse_square_x && legal_move_y == mouse_square_y)
				{
					dragging = true;
					dragging_piece = root.board.piece_at(mouse_square_y, mouse_square_x);
					dragging_from_x = mouse_square_x;
					dragging_from_y = mouse_square_y;
					std::cout << "drag...\n";
					return;
				}
			}

			std::cout << "This piece can't move\n";
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

			// Check that [from_x, from_y, to_x, to_y] is a legal move
			for (Node& child_node : root.children)
			{
				const std::string legal_move = child_node.board.move_to_string();
				const auto legal_start_x = char_to_file(legal_move[0]);
				const auto legal_start_y = char_to_rank(legal_move[1]);
				const auto legal_end_x = char_to_file(legal_move[2]);
				const auto legal_end_y = char_to_rank(legal_move[3]);

				if (dragging_from_x == legal_start_x &&
					dragging_from_y == legal_start_y &&
					mouse_square_x == legal_end_x &&
					mouse_square_y == legal_end_y)
				{
					// reset these, because we're about to invalidate them
					parent_of_best_move = nullptr;
					result_of_best_move = nullptr;

					// Move a ply-1 child node to become the root node
					// This preserves the relevant subset of the move graph.
					Node temp = std::move(child_node);
					root = std::move(temp);

					// Make sure all ply-1 child nodes have been generated.
					// A worker thread will almost certainly have generated these by now, but do this anyway
					// for the sake of correctness.
					root.generate_child_boards();
					return;
				}
			}

			std::cout << "(illegal move)\n";
		}

		void handle_events()
		{
			using namespace detail;

			sf::Event event;
			while (window->pollEvent(event))
			{
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

					case sf::Event::KeyPressed:
						if (event.key.code == sf::Keyboard::Key::X) {}
						else if (event.key.code == sf::Keyboard::Key::Y) {}
						else if (event.key.code == sf::Keyboard::Key::Z) {}
						break;

					case sf::Event::Closed:
						window->close();
						break;
				}
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

			// As part of normalizing x/y vs rank/file, make this block unaware of rank/file, just x/y.

			for (int rank = 7; rank >= 0; --rank)
			{
				for (int file = 0; file < 8; ++file)
				{
					const piece piece = root.board.piece_at(rank, file);
					if (piece.is_empty()) continue;

					// If the current piece is being dragged, don't draw it in the original position
					if (dragging && file == dragging_from_x && rank == dragging_from_y)
						continue;

					const float x_pos = board_x + float(file * tile_size_px);
					const float y_pos = board_y + float(rank * tile_size_px);

					render_piece(piece, x_pos, y_pos);
				}
			}

			if (dragging)
			{
				for (const Node& child_node : root.children)
				{
					const std::string move = child_node.board.move_to_string();
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

		void render_right_overlay()
		{
			std::stringstream ss;

			ss << (root.board.white_to_move() ? "White to move\n\n" : "Black to move\n\n");

			ss << n_of_evals << " positions evaluated in " << engine_time << " ms\n";
			// parent_of_best_move is always this->root for now
			if (parent_of_best_move && result_of_best_move)
			{
				ss << "Best move: ";
				move_to_notation(ss, *parent_of_best_move, *result_of_best_move);
				ss << ' ' << result_of_best_move->get_eval() << '\n';
			}
			ss << '\n';

			ss << root.children.size() << " moves:\n";
			for (const Node& child_node : root.children)
			{
				move_to_notation(ss, root, child_node);
				ss << '\n';
			}

			right_overlay.setString(ss.str());
			window->draw(right_overlay);
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

		void worker_thread();

	public:
		void menu();

		void run()
		{
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

		Node root; // move graph, rooted on the current position

		// engine info
		Node* parent_of_best_move = nullptr;
		Node* result_of_best_move = nullptr;
		size_t engine_depth = 0;
		size_t n_of_evals = 0;
		util::timepoint engine_time = 0;

		const color_t human_color = white;

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
