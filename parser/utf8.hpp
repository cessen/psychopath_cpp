#ifndef UTF8_HPP
#define UTF8_HPP

#include <string>
#include <exception>

class utf8_parse_error: std::exception
{
public:
	virtual const char* what() const noexcept {
		return "Invalid UTF8 sequence.";
	}
};

/**
 * Fetches a single, complete UTF8 code point, returning it as a std::string.
 * Returns an empty string on a malformed codepoint.
 *
 * @param in  Reference to a const string iterator where the parsing is to begin.
 * @param end Reference to the corresponding end iterator for the string.
 *
 * Throws a utf8_parse_error exception on malformed utf8 input.
 */
static inline std::string cur_utf8(const std::string::const_iterator& in, const std::string::const_iterator& end)
{
	const unsigned char* c = reinterpret_cast<const unsigned char*>(&(*in));

	if (in == end)
		return std::string("");

	// Determine the length of the encoded codepoint
	int len = 0;
	if (c[0] < 0b10000000)
		len = 1;
	else if (c[0] < 0b11000000)
		throw utf8_parse_error {}; // Malformed: continuation byte as first byte
	else if (c[0] < 0b11100000)
		len = 2;
	else if (c[0] < 0b11110000)
		len = 3;
	else if (c[0] < 0b11111000)
		len = 4;
	else
		throw utf8_parse_error {}; // Malformed: current utf8 standard only allows up to four bytes

	if (len == 0 || len > (end-in))
		throw utf8_parse_error {}; // Malformed: not enough bytes

	// Read the rest of the bytes of the codepoint,
	// making sure they're proper continuation bytes
	for (int i = 1; i < len; ++i) {
		if ((c[i] & 0b11000000) != 0b10000000)
			throw utf8_parse_error {}; // Malformed: not a continuation byte
	}

	// Success!
	return std::string(in, in+len);
}

/**
 * Like cur_utf8, except it advances the string iterator after parsing the token.
 */
static inline std::string next_utf8(std::string::const_iterator& in, const std::string::const_iterator& end)
{
	std::string c = cur_utf8(in, end);

	in += c.length();

	return c;
}

#endif // UTF8_HPP