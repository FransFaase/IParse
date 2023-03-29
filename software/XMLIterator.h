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
	bool isOpenTag(const char *name) { return _state == 'o' && strcmp(tag, name) == 0; }
	bool acceptOpenTag(const char *name) { if (isOpenTag(name)) { next(); return true; } return false; }
	bool isCloseTag() { return _state == 'c'; }
	bool isAttr() { return _state == 'a'; }
	bool isAttr(const char *name) { return _state == 'a' && strcmp(attr, name) == 0; }
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
