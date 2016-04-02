
/* Jim Viebke
Mar 28 2016 */

#include "node.h"

Node::Node(const Board & set_board, unsigned set_my_ply_depth) :
	board(set_board), my_ply_depth(set_my_ply_depth) {}

Node::Node(const std::vector<Piece> & set_board) : board(Board(set_board)), my_ply_depth(1) {}

Node::Node(const Board & set_board) :
	board(set_board), my_ply_depth(1) {}

void Node::generate_ply(const unsigned & depth)
{
	// if we have not reached the ply depth
	if (my_ply_depth <= depth)
	{
		// generate child boards for this position
		const std::list<Board> child_boards = board.get_child_boards();

		// for each child board
		for (const Board & child_board : child_boards)
		{
			// save the child board to its own node
			child_nodes.push_back(Node(child_board, my_ply_depth + 1));

			// make a recursive call to continue populating down the tree
			child_nodes.back().generate_ply(depth);
		}
	}
}

void Node::print_all() const
{
	static std::list<Board> boards;
	boards.push_back(board);

	Board::print_board(boards);
	std::cin.ignore();

	for (const Node & node : child_nodes)
	{
		node.print_all();
	}
	boards.pop_back();
}

void Node::print_size() const
{
	std::map<size_t, size_t> node_counter;
	// hardcode start position
	node_counter[0] = 1;

	size(node_counter);

	for (auto it = node_counter.begin(); it != node_counter.end(); ++it)
		std::cout << "Number of child nodes from ply " << it->first << ": " << it->second << ".\n";
}

void Node::divide() const
{
	std::map<std::string, size_t> move_divide;

	for (const Node & node : child_nodes)
	{
		// count this node's total number of children
		std::map<size_t, size_t> node_counter;
		node.size(node_counter);

		// the last entry will always be 0, so select the size two levels above .end()
		if (node_counter.size() > 1)
			move_divide[node.board.get_move()] = (----node_counter.end())->second;
	}

	for (auto it = move_divide.begin(); it != move_divide.end(); ++it)
		std::cout << it->first << ": " << it->second << std::endl;
}

void Node::size(std::map<size_t, size_t> & node_counter) const
{
	node_counter[my_ply_depth] += child_nodes.size();
	for (const Node & child : child_nodes)
	{
		child.size(node_counter);
	}
}
