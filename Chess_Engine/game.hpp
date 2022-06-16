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

#include "node.h"

namespace detail
{
	constexpr size_t window_width = 1500;
	constexpr size_t window_height = 900;

	constexpr size_t board_x = 50;
	constexpr size_t board_y = 100;

	const sf::Color background = { 70, 70, 70 };
	const sf::Color brown = { 153, 76, 0 }; // brown (ripped from ye internet)
	const sf::Color cream = { 248, 194, 150 }; // cream (ripped from ye internet)

	constexpr size_t tile_size_px = 90;
	constexpr size_t board_size_px = tile_size_px * 8;

	constexpr size_t piece_resolution_px = 150; // use 150x150, the native piece resolution
}

class Game
{
public:
	Game() : node{ Board { layouts::start_board} } {
		sf::ContextSettings settings;
		settings.antialiasingLevel = 8;

		const std::lock_guard<decltype(game_mutex)> lock(game_mutex);
		std::thread([this] { bot_thread(); }).detach();

		window = std::make_unique<sf::RenderWindow>(
			sf::VideoMode((uint32_t)detail::window_width, (uint32_t)detail::window_height),
			"Matrix chess engine",
			sf::Style::Titlebar,
			settings);

		window->setFramerateLimit(60);

		const std::string ARIAL_LOCATION = "C:/Windows/Fonts/Arial.ttf";
		if (!arial.loadFromFile(ARIAL_LOCATION))
		{
			std::cout << "Could not load " << ARIAL_LOCATION << '\n';
			abort();
		}

		node.generate_child_boards();

		legal_marker.setRadius(detail::tile_size_px / 5);
		legal_marker.setPointCount(15);
		legal_marker.setFillColor({ 50, 50, 50, 255 / 2 });

		overlay.setFont(arial);
		overlay.setCharacterSize(20);
		overlay.setFillColor(sf::Color::Black);

		overlay_right.setFont(arial);
		overlay_right.setCharacterSize(20);
		overlay_right.setFillColor(sf::Color::Black);
		overlay_right.setPosition({ detail::board_size_px + detail::board_x * 2, detail::board_y / 2 });

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
		const float scaling = 1.f / 150.f * (float)detail::tile_size_px;

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

		// Again, awkward flipping of x/y axis vs rank/file.
		// Normalize all of this so the top right corner of the board is 0,0.
		if (node.board.is_empty((int)mouse_square_y, (int)mouse_square_x))
		{
			std::cout << "No piece to drag\n";
			return;
		}

		if (!node.board.piece_at((int)mouse_square_y, (int)mouse_square_x).is_color(node.board.get_color_to_move()))
		{
			std::cout << "Not this color's turn\n";
			return;
		}

		// only allow dragging if we can find a legal move starting from the clicked square
		for (const Node& child_node : node.children)
		{
			const std::string legal_move = child_node.board.get_move();
			const size_t legal_move_x = file_to_x(legal_move[0]);
			const size_t legal_move_y = rank_to_y(legal_move[1]);

			if (legal_move_x == mouse_square_x && legal_move_y == mouse_square_y)
			{
				dragging = true;
				dragging_piece = node.board.piece_at((int)mouse_square_y, (int)mouse_square_x);
				dragging_from_x = mouse_square_x;
				dragging_from_y = mouse_square_y;
				std::cout << "drag...\n";
				break;
			}
		}
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

		/*
		In software, we start with x moving right, then y moving down.
		In chess, we start with "rank" moving up, then "file" moving right.

		This requires some awkward translations.

		I would like to normalize all of this to behave like typical X and Y and only ever
		translate out to rank/file as absolutely required for user presentation and move recording. */

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

		// Check that our [from_x, from_y, to_x, to_y] exists as one entry in the legal moves list
		for (const Node& child_node : node.children)
		{
			const std::string legal_move = child_node.board.get_move();
			const size_t legal_start_x = file_to_x(legal_move[0]);
			const size_t legal_start_y = rank_to_y(legal_move[1]);
			const size_t legal_end_x = file_to_x(legal_move[2]);
			const size_t legal_end_y = rank_to_y(legal_move[3]);

			if (dragging_from_x == legal_start_x &&
				dragging_from_y == legal_start_y &&
				mouse_square_x == legal_end_x &&
				mouse_square_y == legal_end_y)
			{
				// Move a ply-1 child node to become the root node
				// This preserves the relevant subset of the move graph.
				node = child_node;

				if (node.board.position.size() != 64)
				{
					std::cout << "Somehow, child node became root note without a valid position.";
				}

				// node.audit_boards();

				// Make sure all ply-1 child nodes have been generated.
				node.generate_child_boards();

				// node.audit_boards();

				// We made a move (and modified the structure we're iterating over), bail
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
				if (event.key.code == sf::Keyboard::Key::X)
				{
					//
				}
				else if (event.key.code == sf::Keyboard::Key::Y)
				{
					//
				}
				else if (event.key.code == sf::Keyboard::Key::Z)
				{
					//
				}
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

		++frame_counter;

		if (mouse_on_board())
		{
			mouse_square_x = (mouse_x - detail::board_x) / detail::tile_size_px;
			mouse_square_y = (mouse_y - detail::board_y) / detail::tile_size_px;
		}
	}

	void render_piece(const Piece& piece, const float x_pos, const float y_pos)
	{
		if (piece.is_white())
		{
			if (piece.is_rook()) { wr.setPosition({ x_pos, y_pos }); window->draw(wr); }
			if (piece.is_knight()) { wn.setPosition({ x_pos, y_pos }); window->draw(wn); }
			if (piece.is_bishop()) { wb.setPosition({ x_pos, y_pos }); window->draw(wb); }
			if (piece.is_queen()) { wq.setPosition({ x_pos, y_pos }); window->draw(wq); }
			if (piece.is_king()) { wk.setPosition({ x_pos, y_pos }); window->draw(wk); }
			if (piece.is_pawn()) { wp.setPosition({ x_pos, y_pos }); window->draw(wp); }
		}
		else
		{
			if (piece.is_rook()) { br.setPosition({ x_pos, y_pos }); window->draw(br); }
			if (piece.is_knight()) { bn.setPosition({ x_pos, y_pos }); window->draw(bn); }
			if (piece.is_bishop()) { bb.setPosition({ x_pos, y_pos }); window->draw(bb); }
			if (piece.is_queen()) { bq.setPosition({ x_pos, y_pos }); window->draw(bq); }
			if (piece.is_king()) { bk.setPosition({ x_pos, y_pos }); window->draw(bk); }
			if (piece.is_pawn()) { bp.setPosition({ x_pos, y_pos }); window->draw(bp); }
		}
	}

	size_t file_to_x(const char c) { return c - 'a'; }
	size_t rank_to_y(const char c) { return 8 - (c - '0'); }

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
				const Piece piece = node.board.piece_at(rank, file);
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
			for (const Node& child_node : node.children)
			{
				const std::string legal_move = child_node.board.get_move();
				const size_t legal_move_x = file_to_x(legal_move[0]);
				const size_t legal_move_y = rank_to_y(legal_move[1]);

				if (dragging_from_x == legal_move_x &&
					dragging_from_y == legal_move_y)
				{
					const size_t legal_move_to_x = file_to_x(legal_move[2]);
					const size_t legal_move_to_y = rank_to_y(legal_move[3]);

					// add half a tile's size, then remove the marker's radius
					const float adjust = float(detail::tile_size_px / 2 - legal_marker.getRadius());

					legal_marker.setPosition(
						float(board_x + detail::tile_size_px * legal_move_to_x + adjust),
						float(board_y + detail::tile_size_px * legal_move_to_y + adjust));

					window->draw(legal_marker);
				}
			}

			render_piece(dragging_piece, float(mouse_x - tile_size_px / 2), float(mouse_y - tile_size_px / 2));
		}
	}

	void render_move_list()
	{
		std::stringstream move_list;

		move_list << (node.board.white_to_move() ? "White to move\n\n" : "Black to move\n\n");

		move_list << node.children.size() << " moves:\n";
		for (const Node& child_node : node.children)
		{
			const std::string legal_move = child_node.board.get_move();
			const int x = (int)file_to_x(legal_move[0]);
			const int y = (int)rank_to_y(legal_move[1]);

			// help; flipping x,y coordinates to y,x is icky
			if (node.board.piece_at(y, x).is_queen()) { move_list << 'Q'; }
			else if (node.board.piece_at(y, x).is_rook()) { move_list << 'R'; }
			else if (node.board.piece_at(y, x).is_bishop()) { move_list << 'B'; }
			else if (node.board.piece_at(y, x).is_knight()) { move_list << 'N'; }
			else if (node.board.piece_at(y, x).is_king())
			{
				// Special logic for castling
				move_list << 'K';
			}
			else if (node.board.piece_at(y, x).is_pawn())
			{
				// Special logic for en-passant captures
			}

			// help; more icky flipping x,y coordinates to y,x
			const int target_x = (int)file_to_x(legal_move[2]);
			const int target_y = (int)rank_to_y(legal_move[3]);
			if (node.board.is_occupied(target_y, target_x))
			{
				if (node.board.piece_at(y, x).is_pawn())
				{
					move_list << legal_move[0];
				}

				move_list << 'x';
			}

			// add the destination coordinates
			move_list << legal_move[2] << legal_move[3];

			if (child_node.board.position.size() != 64)
			{
				std::cout << "Render function found a child node's board with no position.\n";
			}

			// careful - gotta use the child board here, not the current board
			if (child_node.board.is_king_in_check(
				child_node.board.get_color_to_move())) {
				move_list << '+';
			}

			move_list << '\n';
		}

		overlay_right.setString(move_list.str());
		window->draw(overlay_right);
	}

	void render()
	{
		window->clear(detail::background);

		render_game_board();

		std::stringstream debug;
		debug << mouse_x << ", " << mouse_y << '\n';
		if (mouse_on_board())
		{
			debug << mouse_square_x << ", " << mouse_square_y << '\n';
		}
		overlay.setString(debug.str());
		window->draw(overlay);

		render_move_list();
	}

	void bot_thread();

public:
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
	// Used to regulate reading and modification of the game state between the GUI and bot threads.
	std::mutex game_mutex;

	size_t frame_counter = 0;

	int32_t mouse_x = 0, mouse_y = 0;
	size_t mouse_square_x = 0, mouse_square_y = 0;

	bool dragging{ false };
	Piece dragging_piece{}; // default-constructs to "empty"
	size_t dragging_from_x = 0, dragging_from_y = 0;

	std::unique_ptr<sf::RenderWindow> window;
	sf::Font arial;
	sf::Text overlay; // easy way to add text on the screen
	sf::Text overlay_right; // the move list

	Node node; // move graph, rooted on the current position

	const color human_color = color::white;

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
