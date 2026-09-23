#pragma once

#include <string_view>

struct http_parser_url;

namespace core {

class ParseUrl
{
public:
	ParseUrl();
	~ParseUrl();

	int parse(const char * buf, size_t len);
	bool haveParam();
	std::string_view getPath();
	std::string_view getParam();

private:
	struct http_parser_url * m_url;
	const char * m_buff;
};

} // namespace core
