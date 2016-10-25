#ifndef _INCLUDED_LL1PARSER_H
#define _INCLUDED_LL1PARSER_H

#include "ParserGrammar.h"

class ParsedValue;

class LL1Parser : public AbstractParser
{
public:
	bool parse(const TextFileBuffer& textBuffer, Ident root_id, AbstractParseTree& result);
	
private:
	bool parse_term(GrammarTerminal*, AbstractParseTree &rtree);
	bool parse_ws_term(GrammarTerminal*);
	bool parse_ident(GrammarIdent* ident, AbstractParseTree &rtree);
	bool parse_nt(GrammarNonTerminal* non_term, AbstractParseTree &rtree);
	bool parse_or(GrammarOrRule* or_rule, ParsedValue* prev_parts, AbstractParseTree &rtree);
	bool parse_rule(GrammarRule* rule, ParsedValue* prev_parts, Ident tree_name, AbstractParseTree &rtree);
	bool parse_seq(GrammarRule* rule, const char *chain_sym,
				   AbstractParseTree seq, ParsedValue* prev_parts, const Ident tree_name,
	               AbstractParseTree &rtree) ;
	
	TextFileBuffer _text;	

	void expected_string(const char *s, bool is_keyword);
	
	bool _failed;

	int _depth;

};

#endif // _INCLUDED_LL1PARSER_H

