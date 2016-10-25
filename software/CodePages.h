#ifndef _INCLUDED_CODEPAGES_H
#define _INCLUDED_CODEPAGES_H

typedef unsigned long UnicodeChar;

class AbstractCodePage
{
public:
	AbstractCodePage(const UnicodeChar *map);
	bool from(char c, UnicodeChar &codepoint);
	bool to(UnicodeChar codepoint, char &c);
protected:
	const UnicodeChar *_map;
	int _uniform;
};

class CodePage1252 : public AbstractCodePage
{
public:
	CodePage1252();
};

#endif // _INCLUDED_CODEPAGES_H
