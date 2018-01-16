#include <string.h>
#include <stdio.h>
#include "Ident.h"
#include "String.h"
#include "AbstractParseTree.h"

#include "XMLStreamer.h"

struct XMLStreamer::TagState
{
	TagState(const char *n_tag, TagState* n_prev_state) : tag(n_tag), prev_state(n_prev_state) {}
	const char *tag;
	TagState* prev_state;
};

XMLStreamer::XMLStreamer(FILE *f, bool compact /*= true*/, const char *indent_str /*=" "*/) 
 : _f(f), _compact(compact), _indent_str(indent_str), _depth(0), _content(true), _node_content(true), _cur_state(0) {}

void XMLStreamer::addHeader(const char *str) 
{
	fprintf(_f, "<?%s?>", str);
}

void XMLStreamer::addMeta(const char *str, int depth /*= -1*/)
{
	if (!_content)
		fprintf(_f, ">");
	if (depth < 0)
		depth = _depth;
	newline();
	fprintf(_f, "<!%s>", str);
	_content = true;
	_node_content = true;
}

void XMLStreamer::openTag(const char *tag)
{
	if (!_content)
		fprintf(_f, ">");
	newline();
	fprintf(_f, "<%s", tag);
	_depth++;
	_cur_state = new TagState(tag, _cur_state);
	_content = false;
	_node_content = false;
}

void XMLStreamer::addAttribute(const char *name, const char *value)
{
	fprintf(_f, " %s=\"", name);
	encode_string(value);
	fprintf(_f, "\"");
}

void XMLStreamer::addContent(const char *value)
{
	if (!_content)
	{
		fprintf(_f, ">");
		_content = true;
	}
	encode_string(value);
}

void XMLStreamer::closeTag()
{
	if (_depth == 0)
		return;
	_depth--;
	if (_content)
	{
		if (!_compact && _node_content)
			newline();
		fprintf(_f, "</%s>", _cur_state->tag);
	}
	else
		fprintf(_f, "/>");
	_content = true;
	_node_content = true;
	TagState *state = _cur_state;
	_cur_state = _cur_state->prev_state;
	delete state;
}

void XMLStreamer::stream(const AbstractParseTree& tree)
{
	if (tree.isTree("XML"))
	{
		for (AbstractParseTree::iterator it(tree); it.more(); it.next())
			stream(it);
	}
	else if (tree.isTree("?"))
	{
		AbstractParseTree header = tree.part(1);
		if (header.isString())
			addHeader(header.stringValue());
	}
	else if (tree.isTree("!"))
	{
		AbstractParseTree header = tree.part(1);
		if (header.isString())
			addMeta(header.stringValue());
	}
	else if (tree.isTree())
	{
		openTag(tree.type());
		AbstractParseTree::iterator it(tree);
		for (; it.more(); it.next())
		{
			AbstractParseTree child(it);
			if (!child.isTree("="))
				break;

			AbstractParseTree attr_name(child.part(1));
			AbstractParseTree attr_value(child.part(2));
			if (attr_name.isIdent() && attr_value.isString())
				addAttribute(attr_name.identName().val(), attr_value.stringValue());
		}
		for (; it.more(); it.next())
		{
			AbstractParseTree child(it);
			if (child.isString())
				addContent(child.stringValue());
			else if (child.isTree())
				stream(child);
		}
		closeTag();
	}
};


void XMLStreamer::encode_string(const char *s)
{
	for (; *s != '\0'; s++)
		if (*s == '<')
			fprintf(_f, "&lt;");
		else if (*s == '>')
			fprintf(_f, "&gt;");
		else if (*s == '&')
			fprintf(_f, "&amp;");
		else
			fprintf(_f, "%c", *s);
}

void XMLStreamer::newline()
{
	fprintf(_f, "\n");
	for (int i = 0; i < _depth; i++)
		fprintf(_f, "%s", _indent_str);
}
