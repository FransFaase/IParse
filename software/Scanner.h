#ifndef _INCLUDED_SCANNER_H
#define _INCLUDED_SCANNER_H

class TextFileBuffer;
class Grammar;

class AbstractScanner
{
public:
	AbstractScanner() : _grammar(0) {}

	virtual void initScanning(Grammar* grammar) { _grammar = grammar; }
	virtual void skipSpace(TextFileBuffer& text) = 0;
	virtual bool acceptEOF(TextFileBuffer& text) = 0;
	virtual bool acceptLiteral(TextFileBuffer& text, const char*) = 0;
	virtual bool acceptTerminal(TextFileBuffer& text, Ident name, AbstractParseTree& result) = 0;
	virtual bool acceptWhiteSpace(TextFileBuffer& text, Ident name) { return true; }
protected:
	Grammar *_grammar;
};

class BasicScanner : public AbstractScanner
{
public:
	virtual void initScanning(Grammar* grammar);
	virtual void skipSpace(TextFileBuffer& text);
	virtual bool acceptEOF(TextFileBuffer& text);
	virtual bool acceptLiteral(TextFileBuffer& text, const char* sym);
	virtual bool acceptTerminal(TextFileBuffer& text, Ident name, AbstractParseTree& result);
	virtual bool acceptWhiteSpace(TextFileBuffer& text, Ident name);

private:
	bool accept_ident(TextFileBuffer& text, Ident& ident, bool& is_keyword, const char* keyword);
	bool accept_ident(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_string(TextFileBuffer& text, AbstractParseTree& result, bool c_string);
	bool accept_char(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_int(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_double(TextFileBuffer& text, AbstractParseTree& result);

protected:
	TextFilePos _last_space_pos;
	TextFilePos _last_space_end_pos;
private:
	TextFilePos _last_ident_pos;
	TextFilePos _last_ident_end_pos;
	bool _last_ident_is_keyword;
	Ident _last_ident;

	static Ident id_ident;
	static Ident id_string;
	static Ident id_cstring;
	static Ident id_char;
	static Ident id_int;
	static Ident id_double;
};

class WhiteSpaceScanner : public BasicScanner
{
public:
	virtual void skipSpace(TextFileBuffer& text);
	virtual bool acceptTerminal(TextFileBuffer& text, Ident name, AbstractParseTree& result);

private:
	bool accept_whitespace(TextFileBuffer& text, AbstractParseTree& result);
	static Ident id_ws;
	String ws;
};

class ColourCodingScanner : public BasicScanner
{
public:
	virtual bool acceptTerminal(TextFileBuffer& text, Ident name, AbstractParseTree& result);

private:
	bool accept_colourcommand(TextFileBuffer& text, AbstractParseTree& result);
	void accept_colourcode(TextFileBuffer& text, long &code);
	static Ident id_colourcommand;
};

class RawScanner : public AbstractScanner
{
public:
	virtual void skipSpace(TextFileBuffer& text);
	virtual bool acceptEOF(TextFileBuffer& text);
	virtual bool acceptLiteral(TextFileBuffer& text, const char* sym);
	virtual bool acceptTerminal(TextFileBuffer& text, Ident name, AbstractParseTree& result);
	virtual bool acceptWhiteSpace(TextFileBuffer& text, Ident name);
};

class BareScanner : public RawScanner
{
public:
	virtual void initScanning(Grammar* grammar);
	virtual void skipSpace(TextFileBuffer& text);
	virtual bool acceptLiteral(TextFileBuffer& text, const char* sym);
	virtual bool acceptWhiteSpace(TextFileBuffer& text, Ident name);
private:
	TextFilePos _last_space_pos;
	TextFilePos _last_space_end_pos;
	static Ident id_ws;
	static Ident id_nows;
};



#endif // _INCLUDED_SCANNER_H
