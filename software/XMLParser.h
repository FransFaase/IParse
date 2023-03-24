#ifndef _INCLUDED_XMLPARSER_H
#define _INCLUDED_XMLPARSER_H

#include "AbstractParseTree.h"
#include "TextFileBuffer.h"

class XMLParser
{
public:
	bool parse(const TextFileBuffer& textBuffer, AbstractParseTree& result);
};

#endif // _INCLUDED_XMLPARSER_H
