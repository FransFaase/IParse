#ifndef _INCLUDED_ABSTRACTPARSER_H
#define _INCLUDED_ABSTRACTPARSER_H

#include "ParserGrammar.h"
#include "TextFileBuffer.h"

class AbstractScanner;

class AbstractParser : public Grammar
{
public:
	AbstractParser();
	
	void setScanner(AbstractScanner* scanner) { _scanner = scanner; }
	
	void setDebugLevel(bool debug_nt, bool debug_parse, bool debug_scan) { _debug_nt = debug_nt; _debug_parse = debug_parse; _debug_scan = debug_scan; }

	virtual bool parse(const TextFileBuffer& textBuffer, Ident root_id, AbstractParseTree& result) = 0;

	void printExpected(FILE *f, const char* filename, const TextFileBuffer& textBuffer);
	
protected:

	AbstractScanner* _scanner;
	
	typedef struct {
	  const char *sym;
	  Ident in_nt;
	  GrammarRule* rule;
	  bool is_keyword;
	} expect_t;
	expect_t _expect[200];
	int _nr_exp_syms;
	TextFilePos _f_file_pos;
	void expected_string(TextFilePos& pos, const char *s, bool is_keyword);
	Ident _current_nt;
	GrammarRule* _current_rule;
	
	bool _debug_nt;
	bool _debug_parse;
	bool _debug_scan;
};


class ParsedValue
{   
public:
	ParsedValue* prev;
    AbstractParseTree last;
};

/* Special strings for identifiers and context */
extern const char *tt_identdef;
extern const char *tt_identdefadd;
extern const char *tt_identuse;
extern const char *tt_identfield;



#endif // _INCLUDED_ABSTRACTPARSER_H

