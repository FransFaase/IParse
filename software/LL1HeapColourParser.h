#ifndef _INCLUDED_LL1HEAPPARSER_H
#define _INCLUDED_LL1HEAPPARSER_H

#include "ParserGrammar.h"

class AbstractColourAssigner
{
public:
	virtual void operator ()(const TextFilePos& pos, GrammarColourCoding* colour_coding) = 0;
};

class LL1HeapColourParseProcess;

class LL1HeapColourParser : public AbstractParser
{
	friend class LL1HeapColourParseNTProcess;
	friend class LL1HeapColourParseRuleProcess;
	friend class LL1HeapColourParseOrRuleProcess;
	friend class LL1HeapColourParseSeqProcess;
public:
	bool parse(const TextFileBuffer& textBuffer, Ident root_id, AbstractColourAssigner* colour_assigner);
	
private:
	bool parse_term(GrammarTerminal*);
	bool parse_ws_term(GrammarTerminal*);
	bool parse_ident(GrammarIdent* ident);
	
	//void init_solutions();
	//void free_solutions();
	//ParseSolution* find_solution(unsigned long filepos, Ident nt);
	
	TextFileBuffer _text;
	AbstractColourAssigner* _colour_assigner;

	void enterRule();
	void advanced();
	void leaveLeave();
	void setColourCoding(GrammarColourCoding* colour_coding);

	/*
	struct ColourCodingStack
	{
		ColourCodingStack(rammarColourCoding* n_coding, int n_depth, const TextFilePos& n_pos)
		  : n_coding(coding), n_depth(depth), n_pos(pos), processed(false) {}
		GrammarColourCoding* coding;
		int depth;
		TextFilePos pos;
		bool processed;
		ColourCodingStack *parent;
	};
	ColourCodingStack *_colour_coding_stack;
	int _rule_depth;
	*/

	void expected_string(const char *s, bool is_keyword);
	
	//ParseSolution** _solutions;
	int _depth;

	void call(LL1HeapColourParseProcess* parse_process);
	void exit();
	LL1HeapColourParseProcess* _parse_process;
	bool _failed;
};


#endif // _INCLUDED_LL1HEAPPARSER_H

