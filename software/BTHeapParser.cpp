#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Ident.h"
#include "String.h"
#include "AbstractParseTree.h"
#include "TextFileBuffer.h"
#include "Scanner.h"
#include "AbstractParser.h"
#include "ParseSolution.h"
#include "BTHeapParser.h"


#define DEBUG_ENTER(X) if (_parser->_debug_parse) { DEBUG_TAB; printf("Enter: %s", X); _parser->_depth += 2; }
#define DEBUG_ENTER_P1(X,A) if (_parser->_debug_parse) { DEBUG_TAB; printf("Enter: "); printf(X,A); _parser->_depth += 2; }
#define DEBUG_EXIT(X) if (_parser->_debug_parse) { _parser->_depth -=2; DEBUG_TAB; printf("Leave: %s", X); }
#define DEBUG_EXIT_P1(X,A) if (_parser->_debug_parse) { _parser->_depth -=2; DEBUG_TAB; printf("Leave: "); printf(X,A); }
#define DEBUG_TAB if (_parser->_debug_parse) printf("%*.*s", _parser->_depth, _parser->_depth, "")
#define DEBUG_NL if (_parser->_debug_parse) printf("\n")
#define DEBUG_PT(X) if (_parser->_debug_parse) X.print(stdout, true)
#define DEBUG_PO(X) if (_parser->_debug_parse && X != 0) X->print(stdout)
#define DEBUG_PR(X) if (_parser->_debug_parse && X != 0) X->print(stdout)
#define DEBUG_(X)  if (_parser->_debug_parse) printf(X)
#define DEBUG_P1(X,A) if (_parser->_debug_parse) printf(X,A)



BTHeapParser::BTHeapParser()
{
	_solutions = 0;
	_parse_function = 0;
}

bool BTHeapParser::parse_term( GrammarTerminal* term, AbstractParseTree &rtree )
{
	TextFilePos start_pos = _text;

	if (_debug_scan)
		printf("%d.%d: acceptTerminal(%s)\n", start_pos.line(), start_pos.column(), term->name.val());
	
	bool try_it = _scanner->acceptTerminal(_text, term->name, rtree);
	if (!try_it)
		expected_string(term->name.val(), false);

	if (_debug_scan)
	{
		if (try_it)
		{
			printf("%d.%d: acceptTerminal(%s) : ", start_pos.line(), start_pos.column(), term->name.val());
			rtree.print(stdout, true);
			printf("\n");
	    }
	    else
	    	printf("%d.%d: acceptTerminal(%s) failed at '%s'\n", start_pos.line(), start_pos.column(), term->name.val(), _text.start());
	}
	
	return try_it;
}

bool BTHeapParser::parse_ws_term( GrammarTerminal* term )
{
	TextFilePos start_pos = _text;

	if (_debug_scan)
		printf("%d.%d: acceptWhiteSpace(%s)\n", start_pos.line(), start_pos.column(), term->name.val());
	
	bool try_it = _scanner->acceptWhiteSpace(_text, term->name);
	if (!try_it)
		expected_string(term->name.val(), false);

	if (_debug_scan)
	{
		if (try_it)
			printf("%d.%d: acceptWhiteSpacel(%s)\n", start_pos.line(), start_pos.column(), term->name.val());
	    else
	    	printf("%d.%d: acceptWhiteSpacel(%s) failed at '%s'\n", start_pos.line(), start_pos.column(), term->name.val(), _text.start());
	}
	
	return try_it;
}


bool BTHeapParser::parse_ident(GrammarIdent* ident, AbstractParseTree &rtree)
{	
	AbstractParseTree tree;
	bool try_it = _scanner->acceptTerminal(_text, ident->terminal->name, tree);
	if (!try_it)
		expected_string(ident->terminal->name.val(), false);
	
	if (try_it)
    {   switch(ident->kind)
        {   case IK_IDENTDEF:
                rtree.createTree(tt_identdef);
                break;
            case IK_IDENTDEFADD:
                rtree.createTree(tt_identdefadd);
                break;
            case IK_IDENTUSE:
                rtree.createTree(tt_identuse);
                break;
            case IK_IDENTFIELD:
                rtree.createTree(tt_identfield);
                break;
        }
        rtree.appendChild(tree);
        rtree.appendChild(AbstractParseTree(Ident(ident->ident_class)));
    }

	return try_it;
}

class ParseFunction
{
	friend class BTHeapParser;
public:
	ParseFunction(BTHeapParser* parser) : _state(0), _parser(parser), _parent_function(0) {}
	virtual ~ParseFunction() {}
	virtual void execute() = 0;
protected:
	int _state;
	BTHeapParser* _parser;
private:
	ParseFunction* _parent_function;
};

class ParseNTFunction : public ParseFunction
{
public:
	ParseNTFunction(BTHeapParser* parser, GrammarNonTerminal* non_term, AbstractParseTree& rtree, bool &result)
		: ParseFunction(parser), _non_term(non_term), _rtree(rtree), _result(result) {}

	virtual void execute();

private:
	// parametes:
	GrammarNonTerminal* _non_term;
	AbstractParseTree& _rtree;
	bool &_result;
	// locals:
	GrammarOrRule* _or_rule;
	Ident _surr_nt;
	Ident _nt;
	ParseSolution* _sol;
	ParseFunction* _sub_function;
	bool _sub_result;
    ParsedValue _val;
};

class ParseOrRuleFunction : public ParseFunction
{
public:
	ParseOrRuleFunction(BTHeapParser* parser, GrammarOrRule* or_rule, ParsedValue* prev_parts, AbstractParseTree &rtree, bool &result)
		: ParseFunction(parser), _or_rule(or_rule), _prev_parts(prev_parts), _rtree(rtree), _result(result) {}

	virtual void execute();

private:
	// parameters:
	GrammarOrRule* _or_rule;
	ParsedValue* _prev_parts;
	AbstractParseTree &_rtree;
	bool &_result;
	// locals:
	bool _sub_result;
	ParseFunction* _sub_function;
};


class ParseRuleFunction : public ParseFunction
{
public:
	ParseRuleFunction(BTHeapParser* parser, GrammarRule* rule, ParsedValue* prev_parts, const Ident tree_name, AbstractParseTree &rtree, bool &result)
		: ParseFunction(parser), _rule(rule), _prev_parts(prev_parts), _tree_name(tree_name), _rtree(rtree), _result(result) {}

	virtual void execute();

private:
	// parameters:
	GrammarRule* _rule;
	ParsedValue* _prev_parts;
	const Ident _tree_name;
	AbstractParseTree &_rtree;
	bool &_result;
	// locals
	bool _optional;
	bool _avoid;
	bool _sequential;
    const char *_chain_sym;
    size_t *_last_fail_pos;
    TextFilePos _sp;
    bool _try_it;
	AbstractParseTree _t;
    TextFilePos _start_pos;
    bool _is_terminal;
	ParseFunction* _sub_function;
	bool _sub_result;
	AbstractParseTree _seq;
	ParsedValue _val;
};

class ParseSeqFunction : public ParseFunction
{
public:
	ParseSeqFunction(BTHeapParser* parser, GrammarRule* rule, const char *chain_sym,
					AbstractParseTree seq, ParsedValue* prev_parts, const Ident tree_name,
					AbstractParseTree &rtree, bool &result)
		: ParseFunction(parser), _rule(rule), _chain_sym(chain_sym), _seq(seq), _prev_parts(prev_parts), _tree_name(tree_name), _rtree(rtree), _result(result) {}

	virtual void execute();

private:
	// parameters:
	GrammarRule* _rule;
	const char *_chain_sym;
    AbstractParseTree _seq;
	ParsedValue* _prev_parts; 
	const Ident _tree_name;
    AbstractParseTree &_rtree;
	bool &_result;
	// locals:
	TextFilePos _sp;
	bool _try_it;
	AbstractParseTree _t;
    TextFilePos _start_pos;
	ParsedValue _val;
	ParseFunction* _sub_function;
	bool _sub_result;
};

void ParseNTFunction::execute()
{
	switch (_state) {
		case 1: goto state1;
		case 2: goto state2;
	}

	_surr_nt = _parser->_current_nt;
	_nt = _non_term->name;
	_sol = _parser->find_solution(_parser->_text.position(), _nt);

	DEBUG_ENTER_P1("parse_nt(%s)", _nt.val()); DEBUG_NL;

	if (_sol->success == s_success)
	{
		DEBUG_EXIT_P1("parse_nt(%s) SUCCESS", _nt.val());  DEBUG_NL;
		_rtree = _sol->result;
		_parser->_text = _sol->sp;
		_result = true;
		_parser->exit();
		return;
	}
	else if (_sol->success == s_fail)
	{
		DEBUG_EXIT_P1("parse_nt(%s) FAIL", _nt.val());  DEBUG_NL;
		_result = false;
		_parser->exit();
		return;
	}
    _sol->success = s_fail; // To prevent indirect left-recurrence

	_parser->_current_nt = _nt;

	if (_parser->_debug_nt)
	{   printf("%*.*s", _parser->_depth, _parser->_depth, "");
		printf("Enter: %s\n", _nt.val());
		_parser->_depth += 2; 
	}

	for (_or_rule = _non_term->first; _or_rule != 0; _or_rule = _or_rule->next )
	{
		_sub_function = new ParseRuleFunction(_parser, _or_rule->rule, (ParsedValue*)0, _or_rule->tree_name, _rtree, _sub_result);
		_parser->call(_sub_function);
		_state = 1; return; state1:
		delete _sub_function;
		if (_sub_result)
			break;
	}

	if (_or_rule != 0)
	{
		for(;;)
		{   _val.prev = 0;
			_val.last.attach(_rtree);

			for (_or_rule = _non_term->recursive; _or_rule != 0; _or_rule = _or_rule->next )
			{
				_sub_function = new ParseRuleFunction(_parser, _or_rule->rule, &_val, _or_rule->tree_name, _rtree, _sub_result);
				_parser->call(_sub_function);
				_state = 2; return; state2:
				delete _sub_function;
				if (_sub_result)
					break;
			}

			if (_or_rule == 0)
			{   _rtree = _val.last;
				break;
			}
		}

		DEBUG_EXIT_P1("parse_nt(%s) =", _nt.val());
		DEBUG_PT(_rtree); DEBUG_NL;
		if (_parser->_debug_nt)
		{   _parser->_depth -= 2;
			printf("%*.*s", _parser->_depth, _parser->_depth, "");
			printf("Parsed: %s\n", _nt.val());
		}
		_parser->_current_nt = _surr_nt;
		_sol->result = _rtree;
		_sol->success = s_success;
		_sol->sp = _parser->_text;
		_result = true;
		_parser->exit();
		return;
	}
	DEBUG_EXIT_P1("parse_nt(%s) - failed", _nt.val());  DEBUG_NL;
	if (_parser->_debug_nt)
	{   _parser->_depth -= 2;
		printf("%*.*s", _parser->_depth, _parser->_depth, "");
		printf("Failed: %s\n", _nt.val());
	}
	_parser->_current_nt = _surr_nt;
	_result = false;
	_parser->exit();
}

void ParseOrRuleFunction::execute()
{
	switch(_state) {
		case 1: goto state1;
	}

	DEBUG_ENTER("parse_or: ");
	DEBUG_PO(_or_rule); DEBUG_NL;

	for ( ; _or_rule != 0; _or_rule = _or_rule->next )
	{
		_sub_function = new ParseRuleFunction(_parser, _or_rule->rule, _prev_parts, _or_rule->tree_name, _rtree, _sub_result);
		_parser->call(_sub_function);
		_state = 1; return; state1:
		delete _sub_function;
		if (_sub_result)
		{   DEBUG_EXIT("parse_or = ");
			DEBUG_PT(_rtree); DEBUG_NL;
			_result = true;
			_parser->exit();
			return;
		}
	}
	DEBUG_EXIT("parse_or: failed"); DEBUG_NL;
	_result = false;
	_parser->exit();
}

void ParseRuleFunction::execute()
{   
	switch(_state) {
		case  1: goto state1;
		case  2: goto state2;
		case  3: goto state3;
		case  4: goto state4;
		case  5: goto state5;
		case  6: goto state6;
		case  7: goto state7;
		case  8: goto state8;
		case  9: goto state9;
		case 10: goto state10;
		case 11: goto state11;
		case 12: goto state12;
	}

	DEBUG_ENTER("parse_rule: ");
	DEBUG_PR(_rule); DEBUG_NL;

	/* At the end of the rule: */
	if (_rule == 0)
	{   if (!_tree_name.empty())
		{   /* make tree of previous elements: */
			_rtree.createTree(_tree_name);
			while (_prev_parts)
			{   _rtree.insertChild(_prev_parts->last);
				_prev_parts = _prev_parts->prev;
			}
		}
		else
		{   /* group previous elements into a list, if more than one: */
			if (_prev_parts == 0)
				;
			else if (_prev_parts->prev == 0)
				_rtree = _prev_parts->last;
			else
			{   _rtree.createList();
				while (_prev_parts != 0)
				{   _rtree.insertChild(_prev_parts->last);
					_prev_parts = _prev_parts->prev;
				}
			}
		}
		DEBUG_EXIT("parse_rule = ");
		DEBUG_PT(_rtree); DEBUG_NL;
		_result = true;
		_parser->exit();
		return;
	}

	_optional = _rule->optional;
	_avoid = _rule->avoid;
	_sequential = _rule->sequential;
	_chain_sym = _rule->chain_symbol;

	/* Did we fail the last time at this position? */
	if (_rule->last_fail_pos == _parser->_text.position())
	{
		DEBUG_EXIT("parse_rule - BREAK "); DEBUG_NL;
		_result = false;
		_parser->exit();
		return;
	}
	_last_fail_pos = &_rule->last_fail_pos;

	_sp = _parser->_text;
	_parser->_current_rule = _rule;

	if (_optional && _avoid)
	{    
        /* First element was optional (and should be avoided): */
		_val.prev = _prev_parts;

		_sub_function = new ParseRuleFunction(_parser, _rule->next, &_val, _tree_name, _rtree, _sub_result);
		_parser->call(_sub_function);
		_state = 1; return; state1:
		delete _sub_function;
		if (_sub_result)
		{
			DEBUG_EXIT("parse_rule = ");
			DEBUG_PT(_rtree); DEBUG_NL;
			_result = true;
			_parser->exit();
			return;
		}
		_parser->_text = _sp;
	}

	/* Try to accept first symbol */
	{   _start_pos = _parser->_text;
    	_is_terminal = false;
		_try_it = false;

		switch( _rule->kind )
		{   case RK_T_EOF:
        		_try_it = _parser->_scanner->acceptEOF(_parser->_text);
				if (!_try_it)
					_parser->expected_string("eof", false);
        		if (_try_it)
        		{
					_sub_function = new ParseRuleFunction(_parser, _rule->next, _prev_parts, _tree_name, _rtree, _sub_result);
					_parser->call(_sub_function);
					_state = 2; return; state2:
					delete _sub_function;
					if (_sub_result)
					{   DEBUG_EXIT("parse_rule = ");
						DEBUG_PT(_rtree); DEBUG_NL;
						_result = true;
						_parser->exit();
						return;
					}
				}
				break;
			case RK_TERM:
            	_try_it = _parser->parse_term(_rule->text.terminal, _t);
            	_is_terminal = true;
            	break;
			case RK_IDENT:
            	_try_it = _parser->parse_ident(_rule->text.ident, _t);
            	_is_terminal = true;
            	break;
			case RK_T_OPENCONTEXT:
            	_try_it = true;
            	_t.createOpenContext();
            	break;
			case RK_T_CLOSECONTEXT:
            	_try_it = true;
            	_t.createCloseContext();
            	break;
			case RK_NT:
				_sub_function = new ParseNTFunction(_parser, _rule->text.non_terminal, _t, _try_it);
				_parser->call(_sub_function);
				_state = 3; return; state3:
				delete _sub_function;
				break;
            case RK_LIT:
            case RK_WS_TERM:
			case RK_WS_NT:
			{   if (_rule->kind == RK_LIT)
					_try_it = _parser->_scanner->acceptLiteral(_parser->_text, _rule->str_value);
				else if (_rule->kind == RK_WS_TERM)
					_try_it = _parser->parse_ws_term(_rule->text.terminal);
				else
				{
					_sub_function = new ParseNTFunction(_parser, _rule->text.non_terminal, _t, _try_it);
					_parser->call(_sub_function);
					_state = 4; return; state4:
					delete _sub_function;
				}
				if (_try_it)
				{   if (_sequential)
					{   _seq.createList();

						_sub_function = new ParseSeqFunction(_parser, _rule, _chain_sym, _seq, _prev_parts, _tree_name, _rtree, _sub_result);
						_parser->call(_sub_function);
						_state = 5; return; state5:
						delete _sub_function;
						if (_sub_result)
						{   DEBUG_EXIT("parse_rule = ");
							DEBUG_PT(_rtree); DEBUG_NL;
							_rtree.setLineColumn(_start_pos.line(), _start_pos.column());
							_result = true;
							_parser->exit();
							return;
						}
					}
					else
					{
						_sub_function = new ParseRuleFunction(_parser, _rule->next, _prev_parts, _tree_name, _rtree, _sub_result);
						_parser->call(_sub_function);
						_state = 6; return; state6:
						delete _sub_function;
						if (_sub_result)
						{   DEBUG_EXIT("parse_rule = ");
							DEBUG_PT(_rtree); DEBUG_NL;
							_rtree.setLineColumn(_start_pos.line(), _start_pos.column());
							_result = true;
							_parser->exit();
							return;
						}
					}
				}
				else if (_optional)
				{
					_sub_function = new ParseRuleFunction(_parser, _rule->next, _prev_parts, _tree_name, _rtree, _sub_result);
					_parser->call(_sub_function);
					_state = 7; return; state7:
					delete _sub_function;
					if (_sub_result)
					{   DEBUG_EXIT("parse_rule = ");
						DEBUG_PT(_rtree); DEBUG_NL;
						_rtree.setLineColumn(_start_pos.line(), _start_pos.column());
						_result = true;
						_parser->exit();
						return;
					}
					else
						_parser->expected_string(_rule->str_value, true);
				}
				else
					_parser->expected_string(_rule->str_value, true);
				break;
			}
			case RK_CHARSET:
				_try_it = _rule->text.char_set->contains_char(*_parser->_text);
				if (!_try_it)
					_parser->expected_string("<charset>", false);
				else
				{	_t.createCharAtom(*_parser->_text);
					_parser->_text.next();
					_parser->_scanner->skipSpace(_parser->_text);
				}
				break;
			case RK_AVOID:
				_try_it = !_rule->text.char_set->contains_char(*_parser->_text);
				break;
			case RK_COLOURCODING:
				_try_it = true;
				break;
			case RK_OR_RULE:
				_sub_function = new ParseOrRuleFunction(_parser, _rule->text.or_rules->first, (ParsedValue*)0, _t, _try_it);
				_parser->call(_sub_function);
				_state = 8; return; state8:
				delete _sub_function;
				break;
			case RK_COR_RULE:
				_sub_function = new ParseOrRuleFunction(_parser, _rule->text.or_rules->first, _prev_parts, _rtree, _result);
				_parser->call(_sub_function);
				_state = 9; return; state9:
				delete _sub_function;
				if (_result)
				{   DEBUG_EXIT("parse_rule = ");
					DEBUG_PT(_rtree); DEBUG_NL;
				}
				else
				{
					_parser->_text = _sp;
					*_last_fail_pos = _parser->_text.position();
					DEBUG_EXIT_P1("parse_rule - failed at %ld", _parser->_text.position()); DEBUG_NL;
				}
				_parser->exit();
				return;
				break;
			default:
				_try_it = false;
		}
    

		if (_try_it)
		{   /* We succeded in parsing the first element */

			if (!_t.isEmpty() && _t.line() == 0)
	        	_t.setLineColumn(_start_pos.line(), _start_pos.column());

			if (_sequential)
			{   _seq.createList();
				_seq.appendChild(_t);

				_sub_function = new ParseSeqFunction(_parser, _rule, _chain_sym,
												   _seq, _prev_parts, _tree_name, _rtree, _sub_result);
				_parser->call(_sub_function);
				_state = 10; return; state10:
				delete _sub_function;
				if (_sub_result)
				{   DEBUG_EXIT("parse_rule = ");
					DEBUG_PT(_rtree); DEBUG_NL;
					if (_is_terminal)
                    	_rtree.setLineColumn(_start_pos.line(), _start_pos.column());
					_result = true;
					_parser->exit();
					return;
				}
			}
			else
			{
				_val.last = _t;
				_val.prev = _prev_parts;

				_sub_function = new ParseRuleFunction(_parser, _rule->next, &_val, _tree_name, _rtree, _sub_result);
				_parser->call(_sub_function);
				_state = 11; return; state11:
				delete _sub_function;
				if (_sub_result)
				{   DEBUG_EXIT("parse_rule = ");
					DEBUG_PT(_rtree); DEBUG_NL;
					if (_is_terminal)
                    	_rtree.setLineColumn(_start_pos.line(), _start_pos.column());
					_result = true;
					_parser->exit();
					return;
				}
			}
		}
		_parser->_text = _sp;
	}

	if (_optional && !_avoid)
	{    

		/* First element was optional: */
		_val.prev = _prev_parts;

		_sub_function = new ParseRuleFunction(_parser, _rule->next, &_val, _tree_name, _rtree, _sub_result);
		_parser->call(_sub_function);
		_state = 12; return; state12:
		delete _sub_function;
		if (_sub_result)
		{
			DEBUG_EXIT("parse_rule = ");
			DEBUG_PT(_rtree); DEBUG_NL;
			_result = true;
			_parser->exit();
			return;
		}
		_parser->_text = _sp;
	}


	*_last_fail_pos = _parser->_text.position();
	DEBUG_EXIT_P1("parse_rule - failed at %ld", _parser->_text.position()); DEBUG_NL;
	_result = false;
	_parser->exit();
}

void ParseSeqFunction::execute()
{   
	switch(_state) {
		case 1: goto state1;
		case 2: goto state2;
		case 3: goto state3;
		case 4: goto state4;
		case 5: goto state5;
	}
	
	_try_it = false;

	DEBUG_ENTER("parse_seq: ");
	DEBUG_PR(_rule); DEBUG_NL;

	_sp = _parser->_text;
	_parser->_current_rule = _rule;

	if (_rule->avoid)
	{
		/* should be avoided */
		_val.last = _seq;
		_val.prev = _prev_parts;
		_sub_function = new ParseRuleFunction(_parser, _rule->next, &_val, _tree_name, _rtree, _sub_result);
		_parser->call(_sub_function);
		_state = 1; return; state1:
		delete _sub_function;
		if (_sub_result)
		{
			DEBUG_EXIT("parse_seq = ");
			DEBUG_PT(_rtree); DEBUG_NL;
			_result = true;
			_parser->exit();
			return;
		}
		_parser->_text = _sp;
	}

	/* Try to accept first symbol */
	if (_chain_sym == 0 || *_chain_sym != '\0' || _parser->_scanner->acceptLiteral(_parser->_text, _chain_sym))
	{   _start_pos = _parser->_text;

		switch( _rule->kind )
		{   case RK_T_EOF:
				_try_it = _parser->_scanner->acceptEOF(_parser->_text);
				if (_try_it)
				   _t = Ident("EOF").val();
				break;
			case RK_TERM:
            	_try_it = _parser->parse_term(_rule->text.terminal, _t);
            	break;
			case RK_WS_TERM:
            	_try_it = _parser->parse_ws_term(_rule->text.terminal);
            	break;
			case RK_IDENT:
            	_try_it = _parser->parse_ident(_rule->text.ident, _t);
            	break;
			case RK_NT:
			case RK_WS_NT:
				_sub_function = new ParseNTFunction(_parser, _rule->text.non_terminal, _t, _try_it);
				_parser->call(_sub_function);
				_state = 2; return; state2:
				delete _sub_function;
				if (_rule->kind == RK_WS_NT)
					_t.clear();
				break;
			case RK_LIT:
            	_try_it = _parser->_scanner->acceptLiteral(_parser->_text, _rule->str_value);
            	if (!_try_it)
            		_parser->expected_string(_rule->str_value, true);
				break;
			case RK_CHARSET:
				_try_it = _rule->text.char_set->contains_char(*_parser->_text);
				if (!_try_it)
					_parser->expected_string("<charset>", false);
				else
				{	_t.createCharAtom(*_parser->_text);
					_parser->_text.next();
					_parser->_scanner->skipSpace(_parser->_text);
				}
				break;
			case RK_AVOID:
				_try_it = !_rule->text.char_set->contains_char(*_parser->_text);
				break;
			case RK_COLOURCODING:
				_try_it = true;
				break;
			case RK_OR_RULE:
				_sub_function = new ParseOrRuleFunction(_parser, _rule->text.or_rules->first, (ParsedValue*)0, _t, _try_it);
				_parser->call(_sub_function);
				_state = 3; return; state3:
				delete _sub_function;
				break;
			default:
				_try_it = false;
		} 
    
		if (_try_it)
		{   /* We succeded in parsing the first element */
        
			if (!_t.isEmpty() && _t.line() == 0)
	        	_t.setLineColumn(_start_pos.line(), _start_pos.column());

        	_seq.appendChild(_t);

			_sub_function = new ParseSeqFunction(_parser, _rule, _chain_sym,
											   _seq, _prev_parts, _tree_name, _rtree, _sub_result);
			_parser->call(_sub_function);
			_state = 4; return; state4:
			delete _sub_function;
			if (_sub_result)
			{
				DEBUG_EXIT("parse_seq = ");
				DEBUG_PT(_rtree); DEBUG_NL;
				_result = true;
				_parser->exit();
				return;
			}
			_seq.dropLastChild();
		}
		else if (_chain_sym != 0)
        	_parser->expected_string(_chain_sym, true);
		_parser->_text = _sp;
	}

	if (!_rule->avoid)
	{
		/* should not be avoided */
		_val.last = _seq;
		_val.prev = _prev_parts;
		_sub_function = new ParseRuleFunction(_parser, _rule->next, &_val, _tree_name, _rtree, _sub_result);
		_parser->call(_sub_function);
		_state = 5; return; state5:
		delete _sub_function;
		if (_sub_result)
		{
			DEBUG_EXIT("parse_seq = ");
			DEBUG_PT(_rtree); DEBUG_NL;
			_result = true;
			_parser->exit();
			return;
		}
		_parser->_text = _sp;
	}

	DEBUG_EXIT_P1("parse_seq - failed at %ld", _parser->_text.position()); DEBUG_NL;
	_result = false;
	_parser->exit();
}

void BTHeapParser::init_solutions()
{	unsigned long i;

	_solutions = new ParseSolution*[_text.length()+1];
	for (i = 0; i < _text.length()+1; i++)
		_solutions[i] = 0;
}

void BTHeapParser::free_solutions()
{
	unsigned long i;

	for (i = 0; i < _text.length()+1; i++)
	{	ParseSolution* sol = _solutions[i];

		while (sol != 0)
		{	ParseSolution* next_sol = sol->next;
			delete sol;
			sol = next_sol;
		}
  	}
	delete[] _solutions;
	_solutions = 0;
}

ParseSolution* BTHeapParser::find_solution(unsigned long filepos, Ident nt)
{
	ParseSolution* sol;

	if (filepos > _text.length())
		filepos = _text.length();

	for (sol = _solutions[filepos]; sol != 0; sol = sol->next)
		if ( sol->nt == nt )
		 	return sol;

	sol = new ParseSolution;
	sol->next = _solutions[filepos];
	sol->nt = nt;
	sol->success = s_unknown;
	_solutions[filepos] = sol;

	return sol;
}

void BTHeapParser::expected_string(const char *s, bool is_keyword)
{
	AbstractParser::expected_string(_text, s, is_keyword);
}

void BTHeapParser::call(ParseFunction *parse_function)
{
	parse_function->_parent_function = _parse_function;
	_parse_function = parse_function;
}

void BTHeapParser::exit()
{
	_parse_function = _parse_function->_parent_function;
}

bool BTHeapParser::parse(const TextFileBuffer& textBuffer, Ident root_id, AbstractParseTree& rtree)
{
	_depth = 0;
	_text = textBuffer;
	_f_file_pos = _text;
	_nr_exp_syms = 0;
	
	init_solutions();

	_scanner->initScanning(this);
	_scanner->skipSpace(_text);
	bool try_it = false;
	ParseFunction* _root = new ParseNTFunction(this, findNonTerminal(root_id), rtree, try_it);
	call(_root);
	while (_parse_function != 0)
		_parse_function->execute();
	delete _root;

	free_solutions();
		
	return try_it;
}

#undef DEBUG_ENTER
#undef DEBUG_ENTER_P1
#undef DEBUG_EXIT
#undef DEBUG_EXIT_P1
#undef DEBUG_TAB
#undef DEBUG_NL
#undef DEBUG_PT
#undef DEBUG_PO
#undef DEBUG_PR
#undef DEBUG_
#undef DEBUG_P1
