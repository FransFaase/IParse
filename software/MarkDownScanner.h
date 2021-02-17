#ifndef _INCLUDED_MARKDOWNSCANNER_H
#define _INCLUDED_MARKDOWNSCANNER_H

class MarkDownCScanner : public AbstractScanner
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
	bool accept_macro_ident(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_string(TextFileBuffer& text, AbstractParseTree& result, bool c_string);
	bool accept_char(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_int(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_double(TextFileBuffer& text, AbstractParseTree& result);
	bool accept_macro_def(TextFileBuffer& text, AbstractParseTree& result);

protected:
	TextFilePos _last_space_pos;
	TextFilePos _last_space_end_pos;
private:
	TextFilePos _last_ident_pos;
	TextFilePos _last_ident_end_pos;
	bool _last_ident_is_keyword;
	Ident _last_ident;
	bool _in_text;

	static Ident id_ident;
	static Ident id_macro_ident;
	static Ident id_string;
	static Ident id_cstring;
	static Ident id_char;
	static Ident id_int;
	static Ident id_double;
	static Ident id_macro_def;
};


#endif // _INCLUDED_MARKDOWNSCANNER_H
