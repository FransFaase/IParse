#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Ident.h"
#include "AbstractParseTree.h"
#include "TextFileBuffer.h"
#include "Scanner.h"
#include "AbstractParser.h"
#include "ParseSolution.h"
#include "LL1HeapColourParser.h"


#define DEBUG_ENTER(X) if (_parser->_debug_parse) { DEBUG_TAB; printf("Enter: %s", X); _parser->_depth += 2; }
#define DEBUG_ENTER_P1(X,A) if (_parser->_debug_parse) { DEBUG_TAB; printf("Enter: "); printf(X,A); _parser->_depth += 2; }
#define DEBUG_EXIT(X) if (_parser->_debug_parse) { _parser->_depth -=2; DEBUG_TAB; printf("Leave: %s", X); }
#define DEBUG_EXIT_P1(X,A) if (_parser->_debug_parse) { _parser->_depth -=2; DEBUG_TAB; printf("Leave: "); printf(X,A); }
#define DEBUG_TAB if (_parser->_debug_parse) printf("%*.*s", _parser->_depth, _parser->_depth, "")
#define DEBUG_NL if (_parser->_debug_parse) printf("\n")
#define DEBUG_PT(X) if (_parser->_debug_parse) X.print(stdout, true)
#define DEBUG_PO(X) if (_parser->_debug_parse) X->print(stdout)
#define DEBUG_PR(X) if (_parser->_debug_parse) X->print(stdout)
#define DEBUG_(X)  if (_parser->_debug_parse) printf(X)
#define DEBUG_P1(X,A) if (_parser->_debug_parse) printf(X,A)



bool LL1HeapColourParser::parse_term( GrammarTerminal* term )
{
	TextFilePos start_pos = _text;

	if (_debug_scan)
		printf("%d.%d: acceptTerminal(%s)\n", start_pos.line(), start_pos.column(), term->name.val());
	
	AbstractParseTree tree;
	bool try_it = _scanner->acceptTerminal(_text, term->name, tree);
	if (!try_it)
		expected_string(term->name.val(), false);

	if (_debug_scan)
	{
		if (try_it)
		{
			printf("%d.%d: acceptTerminal(%s)\n", start_pos.line(), start_pos.column(), term->name.val());
	    }
	    else
	    	printf("%d.%d: acceptTerminal(%s) failed at '%s'\n", start_pos.line(), start_pos.column(), term->name.val(), _text.start());
	}
	
	return try_it;
}

bool LL1HeapColourParser::parse_ws_term( GrammarTerminal* term )
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
       
bool LL1HeapColourParser::parse_ident(GrammarIdent* ident)
{	
	AbstractParseTree tree;
	bool try_it = _scanner->acceptTerminal(_text, ident->terminal->name, tree);
	if (!try_it)
		expected_string(ident->terminal->name.val(), false);
	
	return try_it;
}

class LL1HeapColourParseProcess
{
	friend class LL1HeapColourParser;
public:
	LL1HeapColourParseProcess(LL1HeapColourParser* parser) : _parser(parser), _state(0), _parent_process(0) {}
	virtual void execute() = 0;
protected:
	int _state;
	LL1HeapColourParser* _parser;
private:
	LL1HeapColourParseProcess* _parent_process;
};

class LL1HeapColourParseNTProcess : public LL1HeapColourParseProcess
{
public:
	LL1HeapColourParseNTProcess(LL1HeapColourParser* parser, GrammarNonTerminal* non_term, bool &result)
		: LL1HeapColourParseProcess(parser), _non_term(non_term), _result(result) {}

	virtual void execute();

private:
	// parametes:
	GrammarNonTerminal* _non_term;
	bool &_result;
	// locals:
	GrammarOrRule* _or_rule;
	Ident _surr_nt;
	Ident _nt;
	//ParseSolution* _sol;
	LL1HeapColourParseProcess* _sub_process;
	bool _sub_result;
};

class LL1HeapColourParseOrRuleProcess : public LL1HeapColourParseProcess
{
public:
	LL1HeapColourParseOrRuleProcess(LL1HeapColourParser* parser, GrammarOrRule* or_rule, bool &result)
		: LL1HeapColourParseProcess(parser), _or_rule(or_rule), _result(result) {}

	virtual void execute();

private:
	// parameters:
	GrammarOrRule* _or_rule;
	bool &_result;
	// locals:
	bool _sub_result;
	LL1HeapColourParseProcess* _sub_process;
};


class LL1HeapColourParseRuleProcess : public LL1HeapColourParseProcess
{
public:
	LL1HeapColourParseRuleProcess(LL1HeapColourParser* parser, GrammarRule* rule, bool &result)
		: LL1HeapColourParseProcess(parser), _rule(rule), _result(result) {}

	virtual void execute();

private:
	// parameters:
	GrammarRule* _rule;
	bool &_result;
	// locals
	bool _optional;
	bool _sequential;
    const char *_chain_sym;
    //long *_last_fail_pos;
    //TextFilePos _sp;
    bool _try_it;
    TextFilePos _start_pos;
	LL1HeapColourParseProcess* _sub_process;
	bool _sub_result;
};

class LL1HeapColourParseSeqProcess : public LL1HeapColourParseProcess
{
public:
	LL1HeapColourParseSeqProcess(LL1HeapColourParser* parser, GrammarRule* rule, const char *chain_sym, bool &result)
		: LL1HeapColourParseProcess(parser), _rule(rule), _chain_sym(chain_sym), _result(result) {}

	virtual void execute();

private:
	// parameters:
	GrammarRule* _rule;
	const char *_chain_sym;
	bool &_result;
	// locals:
	//TextFilePos _sp;
	bool _try_it;
    TextFilePos _start_pos;
	LL1HeapColourParseProcess* _sub_process;
	bool _sub_result;
};

void LL1HeapColourParseNTProcess::execute()
{
	switch (_state) {
		case 1: goto state1;
		case 2: goto state2;
	}

	_surr_nt = _parser->_current_nt;
	_nt = _non_term->name;

	_parser->_current_nt = _nt;

	if (_parser->_debug_nt)
	{   printf("%*.*s", _parser->_depth, _parser->_depth, "");
		printf("Enter: %s\n", _nt.val());
		_parser->_depth += 2; 
	}

	for (_or_rule = _non_term->first; _or_rule != 0; _or_rule = _or_rule->next )
	{
		_sub_process = new LL1HeapColourParseRuleProcess(_parser, _or_rule->rule, _sub_result);
		_parser->call(_sub_process);
		_state = 1; return; state1:
		delete _sub_process;
		if (_sub_result)
			break;
	}

	if (_or_rule != 0)
	{
		for(;;)
		{   for (_or_rule = _non_term->recursive; _or_rule != 0; _or_rule = _or_rule->next )
			{
				_sub_process = new LL1HeapColourParseRuleProcess(_parser, _or_rule->rule, _sub_result);
				_parser->call(_sub_process);
				_state = 2; return; state2:
				delete _sub_process;
				if (_sub_result)
					break;
			}

			if (_or_rule == 0)
				break;
		}

		DEBUG_EXIT_P1("parse_nt(%s)", _nt.val()); DEBUG_NL;
		if (_parser->_debug_nt)
		{   _parser->_depth -= 2;
			printf("%*.*s", _parser->_depth, _parser->_depth, "");
			printf("Parsed: %s\n", _nt.val());
		}
		_parser->_current_nt = _surr_nt;
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
	//_sol->success = s_fail;
	_result = false;
	_parser->exit();
}

void LL1HeapColourParseOrRuleProcess::execute()
{
	switch(_state) {
		case 1: goto state1;
	}

	DEBUG_ENTER("parse_or: ");
	DEBUG_PO(_or_rule); DEBUG_NL;

	for ( ; _or_rule != 0; _or_rule = _or_rule->next )
	{
		_sub_process = new LL1HeapColourParseRuleProcess(_parser, _or_rule->rule, _sub_result);
		_parser->call(_sub_process);
		_state = 1; return; state1:
		delete _sub_process;
		if (_sub_result)
		{   DEBUG_EXIT("parse_or"); DEBUG_NL;
			_result = true;
			_parser->exit();
			return;
		}
	}
	DEBUG_EXIT("parse_or: failed"); DEBUG_NL;
	_result = false;
	_parser->exit();
}

void LL1HeapColourParseRuleProcess::execute()
{   
	switch(_state) {
		case 2: goto state2;
		case 3: goto state3;
		case 4: goto state4;
		case 5: goto state5;
		case 6: goto state6;
		case 7: goto state7;
		case 8: goto state8;
		case 9: goto state9;
		case 10: goto state10;
	}

	_try_it = false;

	DEBUG_ENTER("parse_rule: ");
	DEBUG_PR(_rule); DEBUG_NL;

	/* At the end of the rule: */
	if (_rule == 0)
	{   DEBUG_EXIT("parse_rule = "); DEBUG_NL;
		_result = true;
		_parser->exit();
		return;
	}

	_optional = _rule->optional,
	_sequential = _rule->sequential;
	_chain_sym = _rule->chain_symbol;

	/* Did we fail the last time at this position? 
	if (_rule->last_fail_pos == _parser->_text.position())
	{
		DEBUG_EXIT("parse_rule - BREAK "); DEBUG_NL;
		_result = false;
		_parser->exit();
		return;
	}
	_last_fail_pos = &_rule->last_fail_pos;
	*/

	//_sp = _parser->_text;
	_parser->_current_rule = _rule;

	/* Try to accept first symbol */
	{   _start_pos = _parser->_text;

		switch( _rule->kind )
		{   case RK_T_EOF:
        		_try_it = _parser->_scanner->acceptEOF(_parser->_text);
				if (!_try_it)
					_parser->expected_string("eof", false);
        		if (_try_it)
        		{
					if (_rule->next == 0)
					{   DEBUG_EXIT("parse_rule"); DEBUG_NL;
						_result = true;
						_parser->exit();
						return;
					}
				}
				break;
			case RK_TERM:
            	_try_it = _parser->parse_term(_rule->text.terminal);
            	break;
			case RK_IDENT:
            	_try_it = _parser->parse_ident(_rule->text.ident);
            	break;
			case RK_T_OPENCONTEXT:
            	_try_it = true;
            	break;
			case RK_T_CLOSECONTEXT:
            	_try_it = true;
            	break;
			case RK_NT:
			case RK_WS_NT:
				_sub_process = new LL1HeapColourParseNTProcess(_parser, _rule->text.non_terminal, _try_it);
				_parser->call(_sub_process);
				_state = 2; return; state2:
				delete _sub_process;
				break;
            case RK_WS_TERM:
            case RK_LIT:
			{   if (_rule->kind == RK_LIT ? _parser->_scanner->acceptLiteral(_parser->_text, _rule->str_value) : _parser->parse_ws_term(_rule->text.terminal))
				{   if (_sequential)
					{   _sub_process = new LL1HeapColourParseSeqProcess(_parser, _rule, _chain_sym, _sub_result);
						_parser->call(_sub_process);
						_state = 3; return; state3:
						delete _sub_process;
						if (_sub_result)
						{   DEBUG_EXIT("parse_rule"); DEBUG_NL;
							_result = true;
							_parser->exit();
							return;
						}
					}
					else
					{
						_sub_process = new LL1HeapColourParseRuleProcess(_parser, _rule->next, _sub_result);
						_parser->call(_sub_process);
						_state = 4; return; state4:
						delete _sub_process;
						if (_sub_result)
						{   DEBUG_EXIT("parse_rule");DEBUG_NL;
							_result = true;
							_parser->exit();
							return;
						}
					}
				}
				else if (_optional)
				{
					_sub_process = new LL1HeapColourParseRuleProcess(_parser, _rule->next, _sub_result);
					_parser->call(_sub_process);
					_state = 5; return; state5:
					delete _sub_process;
					if (_sub_result)
					{   DEBUG_EXIT("parse_rule"); DEBUG_NL;
						_result = true;
						_parser->exit();
						return;
					}
					else
						_parser->expected_string(_rule->str_value.val(), true);
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
				{
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
				_sub_process = new LL1HeapColourParseOrRuleProcess(_parser, _rule->text.or_rules->first, _try_it);
				_parser->call(_sub_process);
				_state = 6; return; state6:
				delete _sub_process;
				break;
			case RK_COR_RULE:
				_sub_process = new LL1HeapColourParseOrRuleProcess(_parser, _rule->text.or_rules->first, _result);
				_parser->call(_sub_process);
				_state = 7; return; state7:
				delete _sub_process;
				if (_result)
				{   DEBUG_EXIT("parse_rule"); DEBUG_NL;
				}
				else
				{
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

			if (_sequential)
			{   _sub_process = new LL1HeapColourParseSeqProcess(_parser, _rule, _chain_sym, _sub_result);
				_parser->call(_sub_process);
				_state = 8; return; state8:
				delete _sub_process;
				if (_sub_result)
				{   DEBUG_EXIT("parse_rule"); DEBUG_NL;
					_result = true;
					_parser->exit();
					return;
				}
			}
			else
			{
				_sub_process = new LL1HeapColourParseRuleProcess(_parser, _rule->next, _sub_result);
				_parser->call(_sub_process);
				_state = 9; return; state9:
				delete _sub_process;
				if (_sub_result)
				{   DEBUG_EXIT("parse_rule = "); DEBUG_NL;
					_result = true;
					_parser->exit();
					return;
				}
				_parser->_failed = true;
			}
			DEBUG_EXIT_P1("parse_rule - failed at %ld", _parser->_text.position()); DEBUG_NL;
			_result = false;
			return;
		}
	}

	if (_optional)
	{    
		//_parser->_text = _sp;

		/* First element was optional: */
		_sub_process = new LL1HeapColourParseRuleProcess(_parser, _rule->next, _sub_result);
		_parser->call(_sub_process);
		_state = 10; return; state10:
		delete _sub_process;
		if (_sub_result)
		{
			DEBUG_EXIT("parse_rule"); DEBUG_NL;
			_result = true;
			_parser->exit();
			return;
		}
		_parser->_failed = true;
	}

	//_parser->_text = _sp;

	//*_last_fail_pos = _parser->_text.position();
	DEBUG_EXIT_P1("parse_rule - failed at %ld", _parser->_text.position()); DEBUG_NL;
	_result = false;
	_parser->exit();
}

void LL1HeapColourParseSeqProcess::execute()
{   
	switch(_state) {
		case 1: goto state1;
		case 2: goto state2;
		case 3: goto state3;
		case 4: goto state4;
	}
	
	_try_it = false;

	DEBUG_ENTER("parse_seq: ");
	DEBUG_PR(_rule); DEBUG_NL;

	//_sp = _parser->_text;
	_parser->_current_rule = _rule;

	/* Try to accept first symbol */
	if (_chain_sym == 0 || *_chain_sym != '\0' || _parser->_scanner->acceptLiteral(_parser->_text, _chain_sym))
	{   _start_pos = _parser->_text;

		switch( _rule->kind )
		{   case RK_T_EOF:
				_try_it = _parser->_scanner->acceptEOF(_parser->_text);
				break;
			case RK_TERM:
            	_try_it = _parser->parse_term(_rule->text.terminal);
            	break;
			case RK_WS_TERM:
            	_try_it = _parser->parse_ws_term(_rule->text.terminal);
            	break;
			case RK_IDENT:
            	_try_it = _parser->parse_ident(_rule->text.ident);
            	break;
			case RK_NT:
			case RK_WS_NT:
				_sub_process = new LL1HeapColourParseNTProcess(_parser, _rule->text.non_terminal, _try_it);
				_parser->call(_sub_process);
				_state = 1; return; state1:
				delete _sub_process;
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
				{
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
				_sub_process = new LL1HeapColourParseOrRuleProcess(_parser, _rule->text.or_rules->first, _try_it);
				_parser->call(_sub_process);
				_state = 2; return; state2:
				delete _sub_process;
				break;
			default:
				_try_it = false;
		} 
    
		if (_try_it)
		{   /* We succeded in parsing the first element */
			_sub_process = new LL1HeapColourParseSeqProcess(_parser, _rule, _chain_sym, _sub_result);
			_parser->call(_sub_process);
			_state = 3; return; state3:
			delete _sub_process;
			if (_sub_result)
			{
				DEBUG_EXIT("parse_seq"); DEBUG_NL;
				_result = true;
				_parser->exit();
				return;
			}
			DEBUG_EXIT_P1("parse_seq - failed at %ld", _parser->_text.position()); DEBUG_NL;
			_result = false;
			return;
		}
		if (_chain_sym != 0)
        	_parser->expected_string(_chain_sym, true);
	}

	//_parser->_text = _sp;

	_sub_process = new LL1HeapColourParseRuleProcess(_parser, _rule->next, _sub_result);
	_parser->call(_sub_process);
	_state = 4; return; state4:
	delete _sub_process;
	if (_sub_result)
	{
		DEBUG_EXIT("parse_seq"); DEBUG_NL;
		_result = true;
		_parser->exit();
		return;
	}

	//_parser->_text = _sp;

	DEBUG_EXIT_P1("parse_seq - failed at %ld", _parser->_text.position()); DEBUG_NL;
	_result = false;
	_parser->exit();
}

/*void LL1HeapColourParser::init_solutions()
{	unsigned long i;

	_solutions = new ParseSolution*[_text.length()+1];
	for (i = 0; i < _text.length()+1; i++)
		_solutions[i] = 0;
}

void LL1HeapColourParser::free_solutions()
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
	delete _solutions;
	_solutions = 0;
}

ParseSolution* LL1HeapColourParser::find_solution(unsigned long filepos, Ident nt)
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
*/

void LL1HeapColourParser::expected_string(const char *s, bool is_keyword)
{
	AbstractParser::expected_string(_text, s, is_keyword);
}

void LL1HeapColourParser::call(LL1HeapColourParseProcess *parse_process)
{
	parse_process->_parent_process = _parse_process;
	_parse_process = parse_process;
}

void LL1HeapColourParser::exit()
{
	_parse_process = _parse_process->_parent_process;
}

bool LL1HeapColourParser::parse(const TextFileBuffer& textBuffer, Ident root_id, AbstractColourAssigner* colour_assigner)
{
	_depth = 0;
	_text = textBuffer;
	_colour_assigner = colour_assigner;
	_f_file_pos = _text;
	_nr_exp_syms = 0;
	
	//init_solutions();
	_failed = false;

	_scanner->initScanning(this);
	_scanner->skipSpace(_text);
	bool try_it = false;
	LL1HeapColourParseProcess* _root = new LL1HeapColourParseNTProcess(this, findNonTerminal(root_id), try_it);
	call(_root);
	while (_parse_process != 0 && _failed)
		_parse_process->execute();
	while (_parse_process != 0)
	{
		LL1HeapColourParseProcess* process = _parse_process;
		_parse_process = process->_parent_process;
		delete process;
	}
	delete _root;

	//free_solutions();
		
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
