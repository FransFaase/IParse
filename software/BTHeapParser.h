#ifndef _INCLUDED_BTHEAPPARSER_H
#define _INCLUDED_BTHEAPPARSER_H

#include "ParserGrammar.h"

class ParseSolution;
class ParsedValue;
class ParseFunction;

class BTHeapParser : public AbstractParser
{
	friend class ParseNTFunction;
	friend class ParseRuleFunction;
	friend class ParseOrRuleFunction;
	friend class ParseSeqFunction;
public:
	BTHeapParser();
	
	bool parse(const TextFileBuffer& textBuffer, Ident root_id, AbstractParseTree& result);
	
private:
	bool parse_term(GrammarTerminal*, AbstractParseTree &rtree);
	bool parse_ws_term(GrammarTerminal*);
	bool parse_ident(GrammarIdent* ident, AbstractParseTree &rtree);
	void init_solutions();
	void free_solutions();
	ParseSolution* find_solution(unsigned long filepos, Ident nt);
	
	TextFileBuffer _text;	

	void expected_string(const char *s, bool is_keyword);
	
	ParseSolution** _solutions;
	int _depth;

	void call(ParseFunction* parse_function);
	void exit();
	ParseFunction* _parse_function;
};

#endif // _INCLUDED_BTHEAPPARSER_H

