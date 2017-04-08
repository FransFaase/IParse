#ifndef _INCLUDED_XMLPARSER_H
#define _INCLUDED_XMLPARSER_H

#include "String.h"
#include "AbstractParseTree.h"
#include "TextFileBuffer.h"

class XMLParser
{
public:
	bool parse(const TextFileBuffer& textBuffer, AbstractParseTree& result);

private:
	TextFileBuffer _text;

	void skip_spaces();
	bool parse_xml(AbstractParseTree& parent);
	void parse_string(char terminator, String &result);
};

#endif // _INCLUDED_XMLPARSER_H
