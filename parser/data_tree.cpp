#include "data_tree.hpp"

#include <fstream>
#include <iostream>

#include "utf8.hpp"

namespace DataTree
{

struct Token {
	enum Type {
	    OPEN_INNER,
	    CLOSE_INNER,
	    OPEN_LEAF,
	    CLOSE_LEAF,
	    TYPE,
	    NAME,
	    END,
	    UNKNOWN
	};

	std::string text {""};
	Type type;
};

/**
 * Returns whether the given utf character is whitespace or not.
 */
static inline bool is_ws_char(const std::string& s)
{
	if (s.length() == 0)
		return false;

	switch (s[0]) {
		case ' ':
		case '\t':
			return true;
		default:
			break;
	}

	return false;
}


/**
 * Returns whether the given utf character is a newline or not.
 */
static inline bool is_nl_char(const std::string& s)
{
	if (s.length() == 0)
		return false;

	switch (s[0]) {
		case '\n':
		case '\r':
			return true;
		default:
			break;
	}

	return false;
}


/**
 * Returns whether the given utf character is a comment starter or not.
 */
static inline bool is_comment_char(const std::string& s)
{
	if (s.length() == 0)
		return false;

	if (s[0] == '#')
		return true;
	else
		return false;
}


/**
 * Returns whether the given utf character is a reserved character or not.
 */
static inline bool is_reserved_char(const std::string& s)
{
	if (s.length() == 0)
		return false;

	switch (s[0]) {
		case '{':
		case '}':
		case '[':
		case ']':
		case '\\':
		case '$':
			return true;
		default:
			break;
	}

	return false;
}

/**
 * Returns whether the given utf character is a legal identifier character or not.
 */
static inline bool is_ident_char(const std::string& s)
{
	if (s.length() == 0)
		return false;

	// Anything that isn't whitespace, reserved, or an operator character
	if (!is_ws_char(s) && !is_nl_char(s) && !is_reserved_char(s) && !is_comment_char(s) && s != "")
		return true;

	return false;
}


bool skip_whitespace(std::string::const_iterator& str_iter, const std::string::const_iterator& str_iter_end)
{
	bool skipped = false;

	std::string cur_c = next_utf8(str_iter, str_iter_end);
	while (is_ws_char(cur_c) || is_nl_char(cur_c)) {
		skipped = true;
		cur_c = next_utf8(str_iter, str_iter_end);
	}
	str_iter -= cur_c.length();

	return skipped;
}


bool skip_comment(std::string::const_iterator& str_iter, const std::string::const_iterator& str_iter_end)
{
	bool skipped = false;

	std::string cur_c = next_utf8(str_iter, str_iter_end);
	if (is_comment_char(cur_c)) {
		while (!is_nl_char(cur_c)) {
			skipped = true;
			cur_c = next_utf8(str_iter, str_iter_end);
		}
	}
	str_iter -= cur_c.length();

	return skipped;
}


bool skip_whitespace_and_comments(std::string::const_iterator& str_iter, const std::string::const_iterator& str_iter_end)
{
	bool skipped = false;
	int skip_consecutive_fail_count = 0;
	while (true) {
		// Skip comments
		if (!skip_whitespace(str_iter, str_iter_end)) {
			skip_consecutive_fail_count++;
		} else {
			skip_consecutive_fail_count = 0;
			skipped = true;
		}

		if (skip_consecutive_fail_count > 1)
			break;

		if (!skip_comment(str_iter, str_iter_end)) {
			skip_consecutive_fail_count++;
		} else {
			skip_consecutive_fail_count = 0;
			skipped = true;
		}

		if (skip_consecutive_fail_count > 1)
			break;
	}

	return skipped;
}


/**
 * Returns the next token as a string.
 */
Token lex_token(std::string::const_iterator& str_iter, const std::string::const_iterator& str_iter_end)
{
	Token token;
	std::string cur_c;

	// Skip whitespace and comments
	skip_whitespace_and_comments(str_iter, str_iter_end);

	// Get next character
	cur_c = next_utf8(str_iter, str_iter_end);

	// If it's a name
	if (cur_c.size() == 1 && cur_c[0] == '$') {
		token.type = Token::NAME;
		do {
			token.text.append(cur_c);
			cur_c = next_utf8(str_iter, str_iter_end);
		} while (is_ident_char(cur_c));
	}

	// If it's a reserved character
	else if (is_reserved_char(cur_c)) {
		token.text = cur_c;
		if (cur_c.size() == 1) {
			switch (cur_c[0]) {
				case '{':
					token.type = Token::OPEN_INNER;
					break;
				case '}':
					token.type = Token::CLOSE_INNER;
					break;
				case '[':
					token.type = Token::OPEN_LEAF;
					break;
				case ']':
					token.type = Token::CLOSE_LEAF;
					break;
			}
		}
		cur_c = next_utf8(str_iter, str_iter_end);
	}

	// If it's an identifier
	else if (is_ident_char(cur_c)) {
		token.type = Token::TYPE;
		do {
			token.text.append(cur_c);
			cur_c = next_utf8(str_iter, str_iter_end);
		} while (is_ident_char(cur_c));
	}

	else if (cur_c == "") {
		token.type = Token::END;
	}

	// If it's anything else
	else {
		token.type = Token::UNKNOWN;
		token.text = cur_c;
		cur_c = next_utf8(str_iter, str_iter_end);
	}

	str_iter -= cur_c.length(); // Backup to last non-consumed character
	return token;
}

/**
 * Returns processed leaf contents as a string.
 *
 * This should be called instead of lex_token() after finding an opening square bracket.
 */
std::string lex_leaf_contents(std::string::const_iterator& str_iter, const std::string::const_iterator& str_iter_end)
{
	std::string contents;
	std::string cur_c = next_utf8(str_iter, str_iter_end);

	while (cur_c.size() > 0) {
		// Skip whitespace and comments
		if (is_ws_char(cur_c) || is_comment_char(cur_c)) {
			str_iter -= cur_c.length();
			skip_whitespace_and_comments(str_iter, str_iter_end);
			contents.append(" ");
			cur_c = next_utf8(str_iter, str_iter_end);
		}

		// End on close bracket
		if (cur_c == "]")
			break;

		contents.append(cur_c);

		cur_c = next_utf8(str_iter, str_iter_end);
	}

	str_iter -= cur_c.length(); // Backup to last non-consumed character
	return contents;
}


Node parse_node(const std::string& type, std::string::const_iterator& str_iter, const std::string::const_iterator& str_iter_end)
{
	Node node;
	Token token;

	node.type = type;

	// Get name if it has one
	token = lex_token(str_iter, str_iter_end);
	if (token.type == Token::NAME) {
		node.name = std::move(token.text);
		token = lex_token(str_iter, str_iter_end);
	}

	// Get contents
	if (token.type == Token::OPEN_INNER) {
		while (true) {
			token = lex_token(str_iter, str_iter_end);

			if (token.type == Token::TYPE) {
				node.children.push_back(parse_node(token.text, str_iter, str_iter_end));
			} else if (token.type == Token::CLOSE_INNER) {
				break;
			} else if (token.type == Token::UNKNOWN) {
				// TODO: ERROR!
				std::cout << "ERROR: Expected node type or closing '}', but got something else.  Aborting." << std::endl;
				return Node();
			} else if (token.type == Token::END) {
				// TODO: ERROR!
				std::cout << "ERROR: Expected node type or closing '}', but reached EOF.  Aborting." << std::endl;
				return Node();
			}
		}
	} else if (token.type == Token::OPEN_LEAF) {
		node.leaf_contents = lex_leaf_contents(str_iter, str_iter_end);

		token = lex_token(str_iter, str_iter_end);

		if (token.type == Token::END) {
			// TODO: ERROR!
			std::cout << "ERROR: Expected closing ']', but reached EOF.  Aborting." << std::endl;
			return Node();
		}
	} else {
		// TODO: ERROR!
		std::cout << "ERROR: Expected opening '{' or '[', but got something else or reached EOF.  Aborting." << std::endl;
		return Node();
	}

	return node;
}

Node build_from_file(const char* file_path)
{
	// Read the file into text
	std::string text;
	std::ifstream in(file_path, std::ios::in | std::ios::binary);
	if (in) {
		in.seekg(0, std::ios::end);
		text.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&(text[0]), text.size());
		in.close();
	} else {
		return Node(); // TODO: throw a proper error
	}

	// Start parsing!
	Node root;
	auto str_iter = text.cbegin();
	auto str_iter_end = text.cend();
	Token token;
	while (true) {
		token = lex_token(str_iter, str_iter_end);

		if (token.type == Token::TYPE) {
			root.children.push_back(parse_node(token.text, str_iter, str_iter_end));
		} else {
			break; // EOF
		}
	}

	return root;
}

} // namespace DataTree
