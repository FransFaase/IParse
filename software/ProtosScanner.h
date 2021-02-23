#ifndef _INCLUDED_PROTOS_SCANNER_H
#define _INCLUDED_PROTOS_SCANNER_H

#include "Scanner.h"

class ProtosScanner : public AbstractScanner
{
public:
	virtual void initScanning(Grammar *grammar);
	virtual void skipSpace(TextFileBuffer& text);
	virtual bool acceptEOF(TextFileBuffer& text);
	virtual bool acceptLiteral(TextFileBuffer& text, const char* sym);
	virtual bool acceptTerminal(TextFileBuffer& text, Ident name, AbstractParseTree& result);

private:
	bool accept_ident(TextFileBuffer& text, Ident& ident);
	bool accept_ident(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_string(TextFileBuffer& text, AbstractParseTree& result, bool c_string);
	//bool accept_char(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_int(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_double(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_newline(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_indent(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_any(TextFileBuffer& text, AbstractParseTree& result);

	TextFilePos _last_space_pos;
	TextFilePos _last_space_end_pos;
	TextFilePos _last_ident_pos;
	TextFilePos _last_ident_end_pos;
	Ident _last_ident;

	static Ident id_ident;
	static Ident id_string;
	static Ident id_int;
	static Ident id_double;
	static Ident id_newline;
	static Ident id_indent;
	static Ident id_any;
};

#endif // _INCLUDED_PROTOS_SCANNER_H
