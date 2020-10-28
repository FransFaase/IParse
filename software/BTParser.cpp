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
#include "BTParser.h"


#define DEBUG_ENTER(X) if (_debug_parse) { DEBUG_TAB; printf("Enter: %s", X); _depth += 2; }
#define DEBUG_ENTER_P1(X,A) if (_debug_parse) { DEBUG_TAB; printf("Enter: "); printf(X,A); _depth += 2; }
#define DEBUG_EXIT(X) if (_debug_parse) { _depth -=2; DEBUG_TAB; printf("Leave: %s", X); }
#define DEBUG_EXIT_P1(X,A) if (_debug_parse) { _depth -=2; DEBUG_TAB; printf("Leave: "); printf(X,A); }
#define DEBUG_TAB if (_debug_parse) printf("%*.*s", _depth, _depth, "")
#define DEBUG_NL if (_debug_parse) printf("\n")
#define DEBUG_PT(X) if (_debug_parse) X.print(stdout, true)
#define DEBUG_PO(X) if (_debug_parse && X != 0) X->print(stdout)
#define DEBUG_PR(X) if (_debug_parse && X != 0) X->print(stdout)
#define DEBUG_(X)  if (_debug_parse) printf(X)
#define DEBUG_P1(X,A) if (_debug_parse) printf(X,A)


BTParser::BTParser()
{
	_solutions = 0;
}

bool BTParser::parse_term( GrammarTerminal* term, AbstractParseTree &rtree )
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

bool BTParser::parse_ws_term( GrammarTerminal* term )
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
       
bool BTParser::parse_ident(GrammarIdent* ident, AbstractParseTree &rtree)
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

bool BTParser::parse_nt( GrammarNonTerminal* non_term, AbstractParseTree& rtree)
{
    GrammarOrRule* or_rule;
    Ident surr_nt = _current_nt;
    Ident nt = non_term->name;
    ParseSolution* sol = find_solution(_text.position(), nt);

    DEBUG_ENTER_P1("parse_nt(%s)", nt.val()); DEBUG_NL;

    if (sol->success == s_success)
    {
        DEBUG_EXIT_P1("parse_nt(%s) SUCCESS", nt.val());  DEBUG_NL;
        rtree = sol->result;
        _text = sol->sp;
        return true;
    }
    else if (sol->success == s_fail)
    {
        DEBUG_EXIT_P1("parse_nt(%s) FAIL", nt.val());  DEBUG_NL;
        return false;
    }

    _current_nt = nt;

    if (_debug_nt)
    {   printf("%*.*s", _depth, _depth, "");
        printf("Enter: %s %d.%d\n", nt.val(), _text.line(), _text.column());
        _depth += 2; 
    }

    for (or_rule = non_term->first; or_rule != 0; or_rule = or_rule->next )
        if (parse_rule(or_rule->rule, (ParsedValue*)0, or_rule->tree_name, rtree))
            break;

    if (or_rule != 0)
    {
        for(;;)
        {   ParsedValue val;
            val.prev = 0;
            val.last.attach(rtree);

            for (or_rule = non_term->recursive; or_rule != 0; or_rule = or_rule->next )
                if (parse_rule(or_rule->rule, &val, or_rule->tree_name, rtree))
                    break;

            if (or_rule == 0)
            {   rtree = val.last;
                break;
            }
        }

        DEBUG_EXIT_P1("parse_nt(%s) =", nt.val());
        DEBUG_PT(rtree); DEBUG_NL;
        if (_debug_nt)
        {   _depth -= 2;
            printf("%*.*s", _depth, _depth, "");
            //printf("Parsed: %s\n", nt.val());
			printf("Parsed: %s %d.%d\n", nt.val(), _text.line(), _text.column());
        }
        _current_nt = surr_nt;
        sol->result = rtree;
        sol->success = s_success;
        sol->sp = _text;
        return true;
    }
    DEBUG_EXIT_P1("parse_nt(%s) - failed", nt.val());  DEBUG_NL;
    if (_debug_nt)
    {   _depth -= 2;
        printf("%*.*s", _depth, _depth, "");
        //printf("Failed: %s\n", nt.val());
		printf("Failed: %s %d.%d\n", nt.val(), _text.line(), _text.column());
    }
    _current_nt = surr_nt;
    sol->success = s_fail;
    return false;
}

bool BTParser::parse_or(GrammarOrRule* or_rule, ParsedValue* prev_parts, AbstractParseTree &rtree)
{
    DEBUG_ENTER("parse_or: ");
    DEBUG_PO(or_rule); DEBUG_NL;

    for ( ; or_rule != 0; or_rule = or_rule->next )
        if (parse_rule(or_rule->rule, prev_parts, or_rule->tree_name, rtree))
        {   DEBUG_EXIT("parse_or = ");
            DEBUG_PT(rtree); DEBUG_NL;
            return true;
        }
    DEBUG_EXIT("parse_or: failed"); DEBUG_NL;
    return false;
}

bool BTParser::parse_rule(GrammarRule* rule, ParsedValue* prev_parts, const Ident tree_name, AbstractParseTree &rtree)
{   
    DEBUG_ENTER("parse_rule: ");
    DEBUG_PR(rule); DEBUG_NL;

    /* At the end of the rule: */
    if (rule == 0)
    {   if (!tree_name.empty())
        {   /* make tree of previous elements: */
            rtree.createTree(tree_name);
            while (prev_parts)
            {   rtree.insertChild(prev_parts->last);
                prev_parts = prev_parts->prev;
            }
        }
        else
        {   /* group previous elements into a list, if more than one: */
            if (prev_parts == 0)
                ;
            else if (prev_parts->prev == 0)
                rtree = prev_parts->last;
            else
            {   rtree.createList();
                while (prev_parts != 0)
                {   rtree.insertChild(prev_parts->last);
                    prev_parts = prev_parts->prev;
                }
            }
        }
        DEBUG_EXIT("parse_rule = ");
        DEBUG_PT(rtree); DEBUG_NL;
        return true;
    }

    bool optional = rule->optional;
	bool avoid = rule->avoid;
    bool sequential = rule->sequential;
    const char *chain_sym = rule->chain_symbol;

    TextFilePos sp = _text;
    _current_rule = rule;

    /* Did we fail the last time at this position? */
    if (rule->last_fail_pos == _text.position())
    {
        DEBUG_EXIT("parse_rule - BREAK "); DEBUG_NL;
        return false;
    }
    long *last_fail_pos = &rule->last_fail_pos;

    if (optional && avoid)
    {    
        /* First element was optional (and should be avoided): */
        ParsedValue val;
        val.prev = prev_parts;

        if (parse_rule(rule->next, &val, tree_name, rtree))
        {
            DEBUG_EXIT("parse_rule = ");
            DEBUG_PT(rtree); DEBUG_NL;
            return true;
        }
		_text = sp;
    }

    /* Try to accept first symbol */
    {   AbstractParseTree t;
    	TextFilePos start_pos = _text;
    	bool is_terminal = false;
		bool try_it = false;
    
        switch( rule->kind )
        {   case RK_T_EOF:
        		try_it = _scanner->acceptEOF(_text);
				if (!try_it)
					expected_string("eof", false);
        		if (try_it)
        		{
                	if (parse_rule(rule->next, prev_parts, tree_name,
                                   rtree))
                    {   DEBUG_EXIT("parse_rule = ");
                        DEBUG_PT(rtree); DEBUG_NL;
                        return true;
                    }
                }
                break;
            case RK_TERM:
            	try_it = parse_term(rule->text.terminal, t);
            	is_terminal = true;
            	break;
            case RK_IDENT:
            	try_it = parse_ident(rule->text.ident, t);
            	break;
            case RK_T_OPENCONTEXT:
            	try_it = true;
            	t.createOpenContext();
            	break;
            case RK_T_CLOSECONTEXT:
            	try_it = true;
            	t.createCloseContext();
            	break;
            case RK_NT:
                try_it = parse_nt(rule->text.non_terminal, t);
                break;
            case RK_WS_TERM:
			case RK_WS_NT:
            case RK_LIT:
			{   if (  rule->kind == RK_LIT 
					? _scanner->acceptLiteral(_text, rule->str_value) 
					: rule->kind == RK_WS_TERM
					? parse_ws_term(rule->text.terminal)
					: parse_nt(rule->text.non_terminal, t))
                {   if (sequential)
                    {   AbstractParseTree seq;
                    	seq.createList();

                        if (parse_seq(rule, chain_sym,
                                      seq, prev_parts, tree_name, rtree))
                        {   DEBUG_EXIT("parse_rule = ");
                            DEBUG_PT(rtree); DEBUG_NL;
                            rtree.setLineColumn(start_pos.line(), start_pos.column());
                            return true;
                        }
                    }
                    else if (parse_rule(rule->next, prev_parts, tree_name,
                                        rtree))
                    {   DEBUG_EXIT("parse_rule = ");
                        DEBUG_PT(rtree); DEBUG_NL;
                        rtree.setLineColumn(start_pos.line(), start_pos.column());
                        return true;
                    }
                }
                else if (optional && parse_rule(rule->next, prev_parts, tree_name, rtree))
                {   DEBUG_EXIT("parse_rule = ");
                    DEBUG_PT(rtree); DEBUG_NL;
                    rtree.setLineColumn(start_pos.line(), start_pos.column());
                    return true;
                }
                else
					expected_string(rule->str_value, true);
                break;
            }
			case RK_CHARSET:
				try_it = rule->text.char_set->contains_char(*_text);
				if (!try_it)
					expected_string("<charset>", false);
				else
				{	t.createCharAtom(*_text);
					_text.next();
					_scanner->skipSpace(_text);
				}
				break;
			case RK_AVOID:
				try_it = !rule->text.char_set->contains_char(*_text);
				break;
			case RK_COLOURCODING:
				try_it = true;
				break;
            case RK_OR_RULE:
                try_it = parse_or(rule->text.or_rules->first, (ParsedValue*)0, t);
                break;
            case RK_COR_RULE:
            	if (parse_or(rule->text.or_rules->first, prev_parts, rtree))
                {   DEBUG_EXIT("parse_rule = ");
                    DEBUG_PT(rtree); DEBUG_NL;
					return true;            		
            	}
            	else
            	{
	    			_text = sp;
					*last_fail_pos = _text.position();
					DEBUG_EXIT_P1("parse_rule - failed at %ld", _text.position()); DEBUG_NL;
					return false;
				}
				break;            	
            default:
                try_it = false;
        }
        

        if (try_it)
        {   /* We succeded in parsing the first element */

			if (!t.isEmpty() && t.line() == 0)
	        	t.setLineColumn(start_pos.line(), start_pos.column());

            if (sequential)
            {   AbstractParseTree seq;
            	seq.createList();
                seq.appendChild(t);

                if (parse_seq(rule, chain_sym,
                              seq, prev_parts, tree_name, rtree))
                {   DEBUG_EXIT("parse_rule = ");
                    DEBUG_PT(rtree); DEBUG_NL;
                    if (is_terminal)
                    	rtree.setLineColumn(start_pos.line(), start_pos.column());
                    return true;
                }
            }
            else
            {
            	ParsedValue val;
                val.last = t;
                val.prev = prev_parts;

                if (parse_rule(rule->next, &val, tree_name, rtree))
                {   DEBUG_EXIT("parse_rule = ");
                    DEBUG_PT(rtree); DEBUG_NL;
                    if (is_terminal)
                    	rtree.setLineColumn(start_pos.line(), start_pos.column());
                    return true;
                }
            }
        }
        _text = sp;
   }

    if (optional && !avoid)
    {    
        /* First element was optional (and not to be avoided): */
        ParsedValue val;
        val.prev = prev_parts;

        if (parse_rule(rule->next, &val, tree_name, rtree))
        {
            DEBUG_EXIT("parse_rule = ");
            DEBUG_PT(rtree); DEBUG_NL;
            return true;
        }
	    _text = sp;
    }

    *last_fail_pos = _text.position();
    DEBUG_EXIT_P1("parse_rule - failed at %ld", _text.position()); DEBUG_NL;
    return false;
}

bool BTParser::parse_seq(GrammarRule* rule, const char *chain_sym,
               AbstractParseTree seq, ParsedValue* prev_parts, const Ident tree_name,
               AbstractParseTree &rtree)
{       
    DEBUG_ENTER("parse_seq: ");
    DEBUG_PR(rule); DEBUG_P1(" at |%s|", _text.start()); DEBUG_NL;
	

	bool avoid = rule->avoid;

    TextFilePos sp = _text;
    _current_rule = rule;

	if (avoid)
	{
		/* should be avoided */
		ParsedValue val;
		val.last = seq;
		val.prev = prev_parts;
		if (parse_rule(rule->next, &val, tree_name, rtree))
		{
			DEBUG_EXIT("parse_seq = ");
			DEBUG_PT(rtree); DEBUG_NL;
			return true;
		}
	    _text = sp;
	}

    /* Try to accept first symbol */
    if (chain_sym == 0 || *chain_sym == '\0' || _scanner->acceptLiteral(_text, chain_sym))
    {   AbstractParseTree t;
    	TextFilePos start_pos = _text;
	    bool try_it = false;

        switch( rule->kind )
        {   case RK_T_EOF:
                try_it = _scanner->acceptEOF(_text);
                if (try_it)
                   t = Ident("EOF").val();
                break;
            case RK_TERM:
            	try_it = parse_term(rule->text.terminal, t);
            	break;
            case RK_WS_TERM:
            	try_it = parse_ws_term(rule->text.terminal);
            	break;
            case RK_IDENT:
            	try_it = parse_ident(rule->text.ident, t);
            	break;
            case RK_NT:
                try_it = parse_nt(rule->text.non_terminal, t);
                break;
			case RK_WS_NT:
                try_it = parse_nt(rule->text.non_terminal, t);
				t.clear();
                break;
            case RK_LIT:
            	try_it = _scanner->acceptLiteral(_text, rule->str_value);
            	if (!try_it)
            		expected_string(rule->str_value, true);
                break;
			case RK_CHARSET:
				try_it = rule->text.char_set->contains_char(*_text);
				if (!try_it)
					expected_string("<charset>", false);
				else
				{	t.createCharAtom(*_text);
					_text.next();
					_scanner->skipSpace(_text);
				}
				break;
			case RK_AVOID:
				try_it = !rule->text.char_set->contains_char(*_text);
				break;
			case RK_COLOURCODING:
				try_it = true;
				break;
            case RK_OR_RULE:
                try_it = parse_or(rule->text.or_rules->first, (ParsedValue*)0, t);
                break;
            case RK_COR_RULE:
            	// should not happen
            	break;
            default:
                try_it = false;
        } 
        
        if (try_it)
        {   /* We succeded in parsing the first element */
            
            if (!t.isEmpty() && t.line() == 0)
	        	t.setLineColumn(start_pos.line(), start_pos.column());
   
        	seq.appendChild(t);

            if (parse_seq(rule, chain_sym,
                          seq, prev_parts, tree_name, rtree))
            {
                DEBUG_EXIT("parse_seq = ");
                DEBUG_PT(rtree); DEBUG_NL;
                return true;
            }
            seq.dropLastChild();
        }
        else if (chain_sym != 0)
        	expected_string(chain_sym, true);
	    _text = sp;
    }

	if (!avoid)
	{
		/* should not be avoided */
		ParsedValue val;
		val.last = seq;
		val.prev = prev_parts;
		if (parse_rule(rule->next, &val, tree_name, rtree))
		{
			DEBUG_EXIT("parse_seq = ");
			DEBUG_PT(rtree); DEBUG_NL;
			return true;
		}
	    _text = sp;
	}

    DEBUG_EXIT_P1("parse_seq - failed at %ld", _text.position()); DEBUG_NL;
    return false;
}

void BTParser::init_solutions()
{	unsigned long i;

	_solutions = new ParseSolution*[_text.length()+1];
	for (i = 0; i < _text.length()+1; i++)
		_solutions[i] = 0;
}

void BTParser::free_solutions()
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

ParseSolution* BTParser::find_solution(unsigned long filepos, Ident nt)
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

void BTParser::expected_string(const char *s, bool is_keyword)
{
	AbstractParser::expected_string(_text, s, is_keyword);
}

bool BTParser::parse(const TextFileBuffer& textBuffer, Ident root_id, AbstractParseTree& result)
{
	_depth = 0;
	_text = textBuffer;
	_f_file_pos = _text;
	_nr_exp_syms = 0;
	
	init_solutions();

	_scanner->initScanning(this);
	_scanner->skipSpace(_text);
	GrammarNonTerminal* root = findNonTerminal(root_id);
	if (root == 0)
	    return false;
	bool try_it = parse_nt(root, result);

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
