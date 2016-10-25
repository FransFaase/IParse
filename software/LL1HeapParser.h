#ifndef _INCLUDED_LL1HEAPPARSER_H
#define _INCLUDED_LL1HEAPPARSER_H

#include "ParserGrammar.h"

class LL1HeapParseProcess;

class LL1HeapParser : public AbstractParser
{
	friend class LL1HeapParseNTProcess;
	friend class LL1HeapParseRuleProcess;
	friend class LL1HeapParseOrRuleProcess;
	friend class LL1HeapParseSeqProcess;
public:
	bool parse(const TextFileBuffer& textBuffer, Ident root_id, AbstractParseTree& result);
	
private:
	bool parse_term(GrammarTerminal*, AbstractParseTree &rtree);
	bool parse_ws_term(GrammarTerminal*);
	bool parse_ident(GrammarIdent* ident, AbstractParseTree &rtree);
	
	//void init_solutions();
	//void free_solutions();
	//ParseSolution* find_solution(unsigned long filepos, Ident nt);
	
	TextFileBuffer _text;	

	void expected_string(const char *s, bool is_keyword);
	
	//ParseSolution** _solutions;
	int _depth;

	void call(LL1HeapParseProcess* parse_process);
	void exit();
	LL1HeapParseProcess* _parse_process;
	bool _failed;
};


#endif // _INCLUDED_LL1HEAPPARSER_H

