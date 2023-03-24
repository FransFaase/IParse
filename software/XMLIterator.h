#ifndef _INCLUDED_XMLITERATOR_H
#define _INCLUDED_XMLITERATOR_H

#include "String.h"
#include "TextFileBuffer.h"

class XMLIterator
{
public:
	XMLIterator(const TextFileBuffer& textBuffer);
	void next();
	bool more() { return _state != '\0'; }
	bool isMeta() { return _state == '?'; }
	bool isComment() { return _state == '!'; }
	bool isOpenTag() { return _state == 'o'; }
	bool isCloseTag() { return _state == 'c'; }
	bool isAttr() { return _state == 'a'; }
	bool isText() { return _state == 't'; }
	char tag[100];
	char attr[100];
	String value;
	int line;
	int column;
	
private:
	char _state;
	TextFileBuffer _text;

	void skip_spaces();
	void parse_string(char terminator);
};

#endif // _INCLUDED_XMLITERATOR_H
