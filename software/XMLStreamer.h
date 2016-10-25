#ifndef _INCLUDED_XMLSTREAMER_H
#define _INCLUDED_XMLSTREAMER_H

class AbstractParseTree;

class XMLStreamer
{
public:

	XMLStreamer(FILE *f, bool compact = true, const char *indent_str = " ");
	void addHeader(const char *str);
	void addMeta(const char *str, int depth = -1);
	void openTag(const char *tag);
	void addAttribute(const char *name, const char *value);
	void addContent(const char *value);
	void closeTag();

	void stream(const AbstractParseTree& tree);

private:
	FILE *_f;
	bool _compact;
	const char* _indent_str;
	int _depth;
	bool _content;
	bool _node_content;
	struct TagState;
	TagState *_cur_state;
	void newline();
	void encode_string(const char *s);
};


#endif // _INCLUDED_XMLSTREAMER_H
