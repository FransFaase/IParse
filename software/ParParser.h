#ifndef _INCLUDED_PARPARSER_H
#define _INCLUDED_PARPARSER_H

#include "ParserGrammar.h"

class Alternative;
class ParsePosition;

class ParParser : public AbstractParser
{
	friend class ParseRootProcess;
	friend class ParseNTProcess;
	friend class ParseRuleProcess;
	friend class ParseOrRuleProcess;
	friend class ParseSeqProcess;
public:
	bool parse(const TextFileBuffer& textBuffer, Ident root_id, AbstractParseTree& result);
	
private:
	bool parse_term(GrammarTerminal*, AbstractParseTree &rtree);
	bool parse_ws_term(GrammarTerminal*);
	bool parse_ident(GrammarIdent* ident, AbstractParseTree &rtree);
	
	TextFileBuffer _text;	

	void insert(Alternative* alt);

	void expected_string(const char *s, bool is_keyword);

	ParsePosition* _parse_position;

	bool              _result;
	AbstractParseTree _result_tree;

	void check(Alternative* root_alt);
};

#endif // _INCLUDED_PARPARSER_H

