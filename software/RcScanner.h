#ifndef _INCLUDED_RCSCANNER_H
#define _INCLUDED_RCSCANNER_H

#include "Scanner.h"

class ResourceScanner : public AbstractScanner
{
public:
	virtual void initScanning(Grammar *grammar);
	virtual void skipSpace(TextFileBuffer& text);
	virtual bool acceptEOF(TextFileBuffer& text);
	virtual bool acceptLiteral(TextFileBuffer& text, const char* sym);
	virtual bool acceptTerminal(TextFileBuffer& text, Ident name, AbstractParseTree& result);
	virtual bool acceptWhiteSpace(TextFileBuffer& text, Ident name);

private:
	bool accept_comment(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_ident(TextFileBuffer& text, Ident& ident, bool& is_keyword, const char* keyword);
	bool accept_ident(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_string(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_char(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_int(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_double(TextFileBuffer& text, AbstractParseTree& result);

	TextFilePos _last_space_pos;
	TextFilePos _last_space_end_pos;
	TextFilePos _last_ident_pos;
	TextFilePos _last_ident_end_pos;
	bool _last_ident_is_keyword;
	Ident _last_ident;
};

#endif // _INCLUDED_RCSCANNER_H
