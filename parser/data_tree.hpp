#ifndef DATA_TREE_HPP
#define DATA_TREE_HPP

#include <string>
#include <vector>

namespace DataTree
{

struct Node {
	std::string type;
	std::string name;
	std::vector<Node> children; // If size is zero, indicates the node is a leaf
	std::string leaf_contents; // Only for leaf nodes
};


/**
 * Builds a data tree from the contents of a file.
 *
 * Returns the root node.
 */
Node build_from_file(const char* file_path);


/**
 * Prints a DataTree to the console, for debugging purposes.
 */
void print_tree(const Node& node, const std::string& indent = "");

}

#endif // DATA_TREE_HPP
