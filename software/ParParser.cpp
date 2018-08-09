#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "Ident.h"
#include "String.h"
#include "AbstractParseTree.h"
#include "TextFileBuffer.h"
#include "Scanner.h"
#include "AbstractParser.h"
#include "ParParser.h"



#define DEBUG_ENTER(X) if (parser->_debug_parse) printf("Enter: %s\n", X);
//#define DEBUG_ENTER(X) if (parser->_debug_parse) { DEBUG_TAB; printf("Enter: %s", X); parser->_depth += 2; }
//#define DEBUG_ENTER_P1(X,A) if (parser->_debug_parse) { DEBUG_TAB; printf("Enter: "); printf(X,A); parser->_depth += 2; }
//#define DEBUG_EXIT(X) if (parser->_debug_parse) { parser->_depth -=2; DEBUG_TAB; printf("Leave: %s", X); }
//#define DEBUG_EXIT_P1(X,A) if (parser->_debug_parse) { parser->_depth -=2; DEBUG_TAB; printf("Leave: "); printf(X,A); }
//#define DEBUG_TAB if (parser->_debug_parse) printf("%*.*s", parser->_depth, parser->_depth, "")
//#define DEBUG_NL if (parser->_debug_parse) printf("\n")
//#define DEBUG_PT(X) if (parser->_debug_parse) X.print(stdout, true)
//#define DEBUG_PO(X) if (parser->_debug_parse) X->print(stdout)
//#define DEBUG_PR(X) if (parser->_debug_parse) X->print(stdout))
//#define DEBUG_(X)  if (parser->_debug_parse) printf(X)
//#define DEBUG_P1(X,A) if (parser->_debug_parse) printf(X,A)

#define DEBUG
#define DEBUG_ALLOC_ALT
//#define TREE_CHECK
#define CHECK  _check();
#define CHILD_CHECK // child->_check();

#ifdef DEBUG
bool detailed_debug = false;
#endif


class MyAlloc
{
public:
	MyAlloc(const char* name) : _name(name), _allocs(0), _allocated(0), _freed(0), _count(0), _reused(0) {}
	void* mynew(size_t size)
	{
		_allocated++;
		if (_allocs != 0)
		{
			_reused++;
			void* result = _allocs;
			_allocs = *(void**)_allocs;
			return result;
		}
		else
		{
			_count++;
			return malloc(size);
		}
	}
	void mydelete(void *old)
	{
		_freed++;
		*(void**)old = _allocs; _allocs = old;
	}
	void print()
	{
		printf("Allocator %s: allocated = %d. freed = %d, left = %d, count = %d, reused = %d\n",
			_name, _allocated, _freed, _allocated - _freed, _count, _reused);
	}
private:
	void* _allocs;
	const char* _name;
	long _allocated;
	long _freed;
	long _count;
	long _reused;
};

#define MYALLOC public: \
static MyAlloc allocs; \
void* operator new(size_t size){ return allocs.mynew(size); } \
void operator delete(void* old) { allocs.mydelete(old); }

class Alternative;

class ParseProcess
{
	// Reference counting is managed by class Alternative.
	friend class Alternative;
public:
	ParseProcess(int state, ParseProcess* parent_process) : _ref_count(1), _state(state), _parent_process(parent_process) {}
	virtual ParseProcess* clone(int state) = 0;
	virtual void execute(ParParser* parser, Alternative* alt) = 0;
	virtual void print(FILE *fout) = 0;
	virtual bool printNT(FILE *fout) { return false; }
	ParseProcess* parent() { return _parent_process; }
protected:
	int _state;
	ParseProcess* _parent_process;
	virtual void destruct() = 0;
private:
	long _ref_count;
};

class ParsePosition
{
	friend class Alternative;
public:
	ParsePosition(ParsePosition* n_next, const TextFilePos& file_pos) : alternatives(0), next(n_next), _file_pos(file_pos) { _tail_alternatives = &alternatives; }
	Alternative* alternatives;
	ParsePosition* next;
	void append(Alternative* alternative);
	const TextFilePos& filePos() { return _file_pos; }
private:
	Alternative** _tail_alternatives;
	TextFilePos _file_pos;
	MYALLOC
};

MyAlloc ParsePosition::allocs("ParsePosition");


int next_alt_id = 0;

class Alternative
{
	friend class ParParser;
public:
	Alternative(Alternative *parent) : _parent(parent), _children(0), _next_sibl(0), _ref_prev_sibl(0), _success(0), _next(0), _ref_prev(0), _process(0), _parse_position(0),
			cr_text(""), cr_grammar_rule(0), _check_state(' ')
	{
		alt_id = next_alt_id++;
		_tail_children = &_children;
		if (_parent != 0)
		{
			_ref_prev_sibl = &_parent->_children;
			_assign_sibl(_parent->_tail_children, this);
			_parent->_tail_children = &_next_sibl;
		}
#ifdef DEBUG_ALLOC_ALT
		_assign_live(&_next_live, all_live);
		_assign_live(&all_live, this);
#endif
		CHECK
	};
	static bool _deleting;
	~Alternative()
	{
		_deleting = true;
#ifdef DEBUG_ALLOC_ALT
		_assign_live(_ref_prev_live, _next_live);
#endif
		releaseProcess();
		remove();
		for (Alternative* child = _children; child != 0; )
		{
			Alternative* next_child = child->_next_sibl;
			delete child;
			child = next_child;
		}
		_deleting = false;
	}
	Alternative* addChild(int state)
	{
		CHECK
		Alternative *child = new Alternative(this);
		if (_process != 0)
		{
			ParseProcess* child_process = _process->clone(state);
			ParseProcess* parent_process = _process->_parent_process;
			if (parent_process != 0)
			{
				child_process->_parent_process = parent_process;			
				parent_process->_ref_count++;
			}
			child->_process = child_process;
		}
		CHECK
		CHILD_CHECK
		return child;
	}
	void releaseProcess()
	{
		CHECK
		for (ParseProcess* process = _process; process != 0;)
		{
			ParseProcess* parent = process->_parent_process;
			if (--process->_ref_count != 0)
				break;
			process->destruct();
			process = parent;
		}
		_process = 0;
		CHECK
	}
	void append(Alternative** &_ref_alternative, ParsePosition* parse_position)
	{
		CHECK
		_parse_position = parse_position;
		_assign(_ref_alternative, this);
		_ref_alternative = &_next;
		CHECK
	}
	void remove()
	{
		CHECK
		if (_ref_prev != 0)
		{
			if (_parse_position != 0 && _parse_position->_tail_alternatives == &_next)
				_parse_position->_tail_alternatives = _ref_prev;
			_parse_position = 0;
			_assign(_ref_prev, _next);
			_ref_prev = 0;
			_next = 0;
		}
		CHECK
	}
	void succeed()
	{
		CHECK
#ifdef DEBUG
		if (detailed_debug)
		{
			printf(" succeed ");
			print(stdout);
			printf("\n");
		}
#endif
		_success++;
		if (_next_sibl == this)
		{
			printf("ERROR13");
			exit(1);
		}

		int success = 0;
		for(Alternative* child = this; child != 0; child = child->_parent)
		{
			printf("child %d\n", child->alt_id);
			success += child->_success;
			if (success == 0)
				break;
#ifndef KILL_SIBLINGS
			// Kill all next siblings
			for (Alternative* sibl = child->_next_sibl; sibl != 0; )
			{
				Alternative* next_sibl = sibl->_next_sibl;
#ifdef DEBUG
				if (detailed_debug)
					printf("  remove[%d]\n", sibl->alt_id);
#endif
				delete sibl;
				sibl = next_sibl;
			}
			child->_next_sibl = 0;
			if (child->_parent != 0)
				child->_parent->_tail_children = &child->_next_sibl;
#endif
			success--;
		}
		CHECK
		_try_reduce();
		CHECK
	}
	void fail()
	{
		CHECK
#ifdef DEBUG
		if (detailed_debug)
		{
			printf(" fail ");
			print(stdout);
			printf("\n");
		}
#endif
		if (_ref_prev_sibl != 0)
		{
			Alternative* next_sibl = _next_sibl;
			if (_parent != 0 && _parent->_tail_children == &_next_sibl)
				_parent->_tail_children = _ref_prev_sibl;
			_assign_sibl(_ref_prev_sibl, _next_sibl);
			if (_next_sibl == this)
			{
				printf("ERROR14");
				exit(1);
			}

			_ref_prev_sibl = 0;
			_next_sibl = 0;
			if (_parent != 0 && _parent->_children == 0)
				_parent->fail();
			else if (next_sibl != 0 && next_sibl->_success > 0)
				next_sibl->_try_reduce();
			delete this;
		}		
	}
	void call(ParseProcess* process)
	{
		CHECK
		process->_parent_process = _process;
		_process = process;
		CHECK
	}
	void finishCall()
	{
		CHECK
		if (_process == 0) { assert(0); return; }

		ParseProcess* parent = _process->_parent_process;
		_process->destruct();
		_process = parent;
		if (_process != 0 && _process->_ref_count > 1)
		{
			_process->_ref_count--;
			_process = _process->clone(_process->_state);
			if (_process->_parent_process != 0)
				_process->_parent_process->_ref_count++;
		}
		CHECK
	}
	inline void execute(ParParser* parser)
	{
		CHECK
		if (_process == 0) { assert(0); return; }

		_process->execute(parser, this);
	}
	inline ParseProcess* process() { return _process; }
	inline AbstractParseTree& returnResult() { return result; }

	AbstractParseTree result;

	void print(FILE* fout, int depth = -1)
	{
		if (depth >= 0)
			fprintf(fout, "\n%*.*s", depth, depth, "");
		fprintf(fout, "[%d]%s ", alt_id, cr_text);
		if (_parse_position != 0)
			fprintf(fout, "%d.%d", _parse_position->_file_pos.line(), _parse_position->_file_pos.column());
		else
			fprintf(fout, "*");
		if (_success > 0)
			fprintf(fout, " (%d success)", _success);
		if (_process != 0)
			_process->print(fout);
		else
		{
			fprintf(fout, " (no process)");
			if (cr_grammar_rule != 0)
				cr_grammar_rule->print(fout);
		}
		
		if (depth >= 0)
			for (Alternative* child = _children; child != 0; child = child->_next_sibl)
				child->print(fout, depth+1);
	}

	int alt_id;
	const char *cr_text;
	GrammarRule* cr_grammar_rule;

private:
	static inline void _assign(Alternative** dest, Alternative* src) { *dest = src; if (src != 0) src->_ref_prev = dest; }
	static inline void _assign_sibl(Alternative** dest, Alternative* src) { *dest = src; if (src != 0) src->_ref_prev_sibl = dest;
		if ((*dest) != 0 && (*dest)->_next_sibl == (*dest))
		{
			printf("ERROR15");
			exit(1);
		}
		if (src != 0 && src->_next_sibl == src)
		{
			printf("ERROR16");
			exit(1);
		}
	}
	void _try_reduce()
	{
		printf("try_reduce\n");
		CHECK
		// If we are the first, take place of parent
		while (_success > 0 && _parent != 0 && _parent->_children == this)
		{
			//printf(" reduced\n");
			Alternative* old_parent = _parent;
			alt_id = old_parent->alt_id;
			cr_text = old_parent->cr_text;
			_success += old_parent->_success;
			_parent = old_parent->_parent;
			_assign_sibl(&_next_sibl, old_parent->_next_sibl);
			if (_next_sibl == this)
			{
				printf("ERROR17");
				exit(1);
			}
			if (old_parent->_ref_prev_sibl == 0)
			{
				printf("ERROR18");
				exit(1);
			}
			else
				_assign_sibl(old_parent->_ref_prev_sibl, this);
			if (_next_sibl == 0)
				_parent->_tail_children = &_next_sibl;
			old_parent->_children = 0;
#ifdef DEBUG
			if (detailed_debug)
				printf("  reduced[%d]\n", old_parent->alt_id);
#endif
			delete old_parent;
			_success--;
			CHECK
		}
	}
	void _kill()
	{
		delete this;
	}

#ifdef DEBUG_ALLOC_ALT
	static Alternative *all_live;
	Alternative *_next_live;
	Alternative **_ref_prev_live;
	static inline void _assign_live(Alternative** dest, Alternative* src) { *dest = src; if (src != 0) src->_ref_prev_live = dest; }
public:
	static void print_live()
	{
		for (Alternative *alt = all_live; alt != 0; alt = alt->_next_live)
		{
			alt->print(stdout,-1);
			printf("\n");
		}
	}
private:
#endif
	void _check()
	{
		if (_deleting)
			return;
		if (_ref_prev_sibl != 0 && (*_ref_prev_sibl) != this)
		{
			printf("ERROR1");
			exit(1);
		}
		if (_next_sibl != 0 && _next_sibl->_ref_prev_sibl != &_next_sibl)
		{
			printf("ERROR2");
			exit(1);
		}
		if (_children != 0)
		{
			if (_children->_ref_prev_sibl != &_children)
			{
				printf("ERROR3");
				exit(1);
			}
			for (Alternative* child = _children; child != 0; child = child->_next_sibl)
			{
				if (child->_parent != this)
				{
					printf("ERROR4");
					exit(1);
				}
				if (child->_next_sibl == 0 && _tail_children != &child->_next_sibl)
				{
					printf("ERROR5");
					exit(1);
				}
			}
		}
		if (_parent != 0)
		{
			bool found_myself = false;
			for (Alternative* sibl = _parent->_children; sibl != 0; sibl = sibl->_next_sibl)
			{
				if (sibl == this)
					found_myself = true;
				if (sibl->_parent != _parent)
				{
					printf("ERROR6");
					exit(1);
				}
				if (sibl->_next_sibl == 0 && _parent->_tail_children != &sibl->_next_sibl)
				{
					printf("ERROR7");
					exit(1);
				}
			}
			if (!found_myself)
			{
				printf("ERROR8");
				exit(1);
			}
		}
	}

	void check()
	{
		Alternative** ref_child = &_children;
		for (; (*ref_child) != 0; ref_child = &(*ref_child)->_next_sibl)
		{
			if ((*ref_child)->_parent != this)
			{
				printf("ERROR9");
				exit(1);
			}
			if ((*ref_child)->_next_sibl != 0 && (*ref_child)->_ref_prev_sibl != ref_child)
			{
				printf("ERROR10");
				exit(1);
			}
			(*ref_child)->check();
		}
		if (ref_child != _tail_children)
		{
			printf("ERROR11");
			exit(1);
		}

		if (_check_state == 'A')
			_check_state = 'F';
	}
			

	Alternative *_parent;
	Alternative *_children;
	Alternative **_tail_children;
	Alternative *_next_sibl;
	Alternative **_ref_prev_sibl;
	ParsePosition *_parse_position;
	Alternative *_next;
	Alternative **_ref_prev;
	int _success;
	ParseProcess* _process;
	char _check_state;
	MYALLOC
};
bool Alternative::_deleting = false;

#ifdef DEBUG_ALLOC_ALT
Alternative *Alternative::all_live = 0;
#endif
MyAlloc Alternative::allocs("Alternative");

void ParsePosition::append(Alternative* alternative)
{
	alternative->append(_tail_alternatives, this);
}


bool ParParser::parse_term( GrammarTerminal* term, AbstractParseTree &rtree )
{
	TextFilePos start_pos = _text;
	//if (_debug_scan)
	//	printf("%d.%d: acceptTerminal(%s)\n", start_pos.line(), start_pos.column(), term->name.val());
	
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

bool ParParser::parse_ws_term( GrammarTerminal* term )
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

bool ParParser::parse_ident(GrammarIdent* ident, AbstractParseTree &rtree)
{	
	TextFilePos start_pos = _text;
	//if (_debug_scan)
	//	printf("%d.%d: acceptTerminal(%s)\n", start_pos.line(), start_pos.column(), ident->terminal->name.val());

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

	if (_debug_scan)
	{
		if (try_it)
		{
			printf("%d.%d: acceptTerminal(%s) : ", start_pos.line(), start_pos.column(), ident->terminal->name.val());
			rtree.print(stdout, true);
			printf("\n");
	    }
	    else
	    	printf("%d.%d: acceptTerminal(%s) failed at '%s'\n", start_pos.line(), start_pos.column(), ident->terminal->name.val(), _text.start());
	}

	return try_it;
}

class ParseRootProcess : public ParseProcess
{
public:
	ParseRootProcess(Ident root_id, int state = 0, ParseProcess* parent_process = 0) 
		: ParseProcess(state, parent_process), _root_id(root_id) {}
	~ParseRootProcess()
	{
		int x = 1;
	}
	virtual ParseProcess* clone(int state)
	{
		return new ParseRootProcess(_root_id, state);
	}

	virtual void execute(ParParser* parser, Alternative* alt);
	virtual void print(FILE *fout);

private:
	// parametes:
	Ident _root_id;
protected:
	virtual void destruct() { delete this; }
	MYALLOC
};

MyAlloc ParseRootProcess::allocs("ParseRootProcess");


class ParseNTProcess : public ParseProcess
{
public:
	ParseNTProcess(GrammarNonTerminal* non_term, int state = 0, ParseProcess* parent_process = 0, Ident nt = Ident(), ParsedValue val = ParsedValue()) 
		: ParseProcess(state, parent_process), _non_term(non_term), _nt(nt), _val(val) {}
	virtual ParseProcess* clone(int state)
	{
		return new ParseNTProcess(_non_term, state, _parent_process, _nt, _val);
	}

	virtual void execute(ParParser* parser, Alternative* alt);
	virtual void print(FILE *fout);
	virtual bool printNT(FILE *fout) { printf("%s", _nt.val()); return true; }

private:
	// parametes:
	GrammarNonTerminal* _non_term;
	// locals:
	//GrammarOrRule* _or_rule;
	Ident _nt;
    ParsedValue _val;
protected:
	virtual void destruct() { delete this; }
	MYALLOC
};

MyAlloc ParseNTProcess::allocs("ParseNTProcess");

class ParseOrRuleProcess : public ParseProcess
{
public:
	ParseOrRuleProcess(GrammarOrRule* or_rule, ParsedValue* prev_parts, int state = 0, ParseProcess* parent_process = 0)
		: ParseProcess(state, parent_process), _or_rule(or_rule), _prev_parts(prev_parts) {}
	virtual ParseProcess* clone(int state)
	{
		return new ParseOrRuleProcess(_or_rule, _prev_parts, state, _parent_process);
	}

	virtual void execute(ParParser* parser, Alternative* alt);
	virtual void print(FILE *fout);

private:
	// parameters:
	GrammarOrRule* _or_rule;
	ParsedValue* _prev_parts;
protected:
	virtual void destruct() { delete this; }
	MYALLOC
};

MyAlloc ParseOrRuleProcess::allocs("ParseOrRuleProcess");


class ParseRuleProcess : public ParseProcess
{
public:
	ParseRuleProcess(GrammarRule* rule, ParsedValue* prev_parts, const Ident tree_name,
					 int state = 0, ParseProcess* parent_process = 0, bool optional = false, bool avoid = false, bool nongreedy = false, bool sequential = false, const char *chain_sym = 0,
					 TextFilePos start_pos = TextFilePos(), bool is_terminal = false)
		: ParseProcess(state, parent_process), _rule(rule), _prev_parts(prev_parts), _tree_name(tree_name),
		  _optional(optional), _avoid(avoid), _nongreedy(nongreedy), _sequential(sequential), _chain_sym(chain_sym),
		  _start_pos(start_pos), _is_terminal(is_terminal) {}
	virtual ParseProcess* clone(int state)
	{
		return new ParseRuleProcess(_rule, _prev_parts, _tree_name,
									state, _parent_process, _optional, _avoid, _nongreedy, _sequential, _chain_sym, _start_pos, _is_terminal);
	}

	virtual void execute(ParParser* parser, Alternative* alt);
	virtual void print(FILE *fout);

private:
	// parameters:
	GrammarRule* _rule;
	ParsedValue* _prev_parts;
	const Ident _tree_name;
	// locals
	bool _optional;
	bool _avoid;
	bool _nongreedy;
	bool _sequential;
    const char *_chain_sym;
    long *_last_fail_pos;
    bool _try_it;
	TextFilePos _start_pos;
	AbstractParseTree _t;
    bool _is_terminal;
	ParsedValue _seq_val;
	ParsedValue _val;
protected:
	virtual void destruct() { delete this; }
	MYALLOC
};

MyAlloc ParseRuleProcess::allocs("ParseRuleProcess");

class ParseSeqProcess : public ParseProcess
{
public:
	ParseSeqProcess(GrammarRule* rule, const char *chain_sym,
					ParsedValue* prev_seq_parts, ParsedValue* prev_parts, const Ident tree_name,
					int state = 0, ParseProcess* parent_process = 0)
		: ParseProcess(state, parent_process), _rule(rule), _chain_sym(chain_sym), _prev_seq_parts(prev_seq_parts), _prev_parts(prev_parts), _tree_name(tree_name) {}
	virtual ParseProcess* clone(int state)
	{
		return new ParseSeqProcess(_rule, _chain_sym, _prev_seq_parts, _prev_parts, _tree_name, state, _parent_process);
	}

	virtual void execute(ParParser* parser, Alternative* alt);
	virtual void print(FILE *fout);

private:
	// parameters:
	GrammarRule* _rule;
	const char *_chain_sym;
    ParsedValue* _prev_seq_parts;
	ParsedValue* _prev_parts; 
	const Ident _tree_name;
	// locals:
	bool _try_it;
	TextFilePos _start_pos;
	AbstractParseTree _t;
	ParsedValue _val;
protected:
	virtual void destruct() { delete this; }
	MYALLOC
};

MyAlloc ParseSeqProcess::allocs("ParseSeqProcess");

void ParseRootProcess::execute(ParParser* parser, Alternative* alt)
{
	//ParseRootProcess* process = (ParseRootProcess*)alt->process();

	switch (_state) {
		case 1: goto state1;
	}
	
	DEBUG_ENTER("Root:start")
	alt->call(new ParseNTProcess(parser->findNonTerminal(_root_id)));
	parser->insert(alt);
	_state = 1; return; state1:
	DEBUG_ENTER("Root:done")
	parser->_result = true;
	parser->_result_tree = alt->result;

	alt->finishCall(); // Just to clean-up a little bit...

	return;
}

void ParseRootProcess::print(FILE* fout)
{
	if (_parent_process != 0)
		_parent_process->print(fout);
	fprintf(fout, " root");
}

void ParseNTProcess::execute(ParParser* parser, Alternative* alt)
{
	//ParseNTProcess* process = (ParseNTProcess*)alt->process();

	switch (_state) {
		case 1: goto state1;
		case 2: goto state2;
	}

	DEBUG_ENTER("NT:init")
	_nt = _non_term->name;

	{
		for (GrammarOrRule* or_rule = _non_term->first; or_rule != 0; or_rule = or_rule->next )
		{
			Alternative* child_alt = alt->addChild(1);
			child_alt->cr_text = "NT_Or";
			child_alt->cr_grammar_rule = or_rule->rule;

			child_alt->call(new ParseRuleProcess(or_rule->rule, (ParsedValue*)0, or_rule->tree_name));
			parser->insert(child_alt);
		}
		alt->releaseProcess();
		return;
	}

state1:
	DEBUG_ENTER("NT:succes")
	alt->succeed();
	// return from call of ParseRuleProcess
	{
		for (GrammarOrRule* or_rule = _non_term->recursive; or_rule != 0; or_rule = or_rule->next )
		{
			Alternative* child_alt = alt->addChild(1);
			child_alt->cr_text = "NT_L_Or";
			child_alt->cr_grammar_rule = or_rule->rule;
			ParseNTProcess* child_process = (ParseNTProcess*)child_alt->process();

			child_process->_val.prev = 0;
			child_process->_val.last = alt->result;

			child_alt->call(new ParseRuleProcess(or_rule->rule, &child_process->_val, or_rule->tree_name));
			parser->insert(child_alt);
		}
		Alternative* child_alt = alt->addChild(2);
		child_alt->cr_text = "NT_done";
		child_alt->result.attach(alt->result);
		parser->insert(child_alt);
		alt->releaseProcess();
		return;
	}

state2:
	DEBUG_ENTER("NT:done")
	alt->succeed();
	alt->finishCall();
	alt->execute(parser);
}

void ParseNTProcess::print(FILE* fout)
{
	if (_parent_process != 0)
		_parent_process->print(fout);
	fprintf(fout, " NT:%s,%d", _non_term->name.val(), _state);
}

void ParseOrRuleProcess::execute(ParParser* parser, Alternative* alt)
{
	//ParseOrRuleProcess* process = (ParseOrRuleProcess*)alt->process();

	switch(_state) {
		case 1: goto state1;
	}

	DEBUG_ENTER("Or:init")
	{
		for (GrammarOrRule* or_rule = _or_rule; or_rule != 0; or_rule = or_rule->next )
		{
			Alternative* child_alt = alt->addChild(1);
			child_alt->cr_text = "Or";
			child_alt->cr_grammar_rule = or_rule->rule;

			child_alt->call(new ParseRuleProcess(or_rule->rule, _prev_parts, or_rule->tree_name));
			parser->insert(child_alt);
		}
		alt->releaseProcess();
		return;
	}

state1:
	DEBUG_ENTER("Or:done")
	alt->succeed();
	alt->finishCall();
	alt->execute(parser);
}

void ParseOrRuleProcess::print(FILE* fout)
{
	if (_parent_process != 0)
		_parent_process->print(fout);
	fprintf(fout, " or:%d", _state);
}

void ParseRuleProcess::execute(ParParser* parser, Alternative* alt)
{
	//ParseRuleProcess* process = (ParseRuleProcess*)alt->process();

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

	DEBUG_ENTER("Rule:init");
	//DEBUG_PR(_rule); DEBUG_NL;

	/* At the end of the rule: */
	if (_rule == 0)
	{   if (!_tree_name.empty())
		{   /* make tree of previous elements: */
			alt->result.createTree(_tree_name);
			while (_prev_parts)
			{   alt->result.insertChild(_prev_parts->last);
				_prev_parts = _prev_parts->prev;
			}
		}
		else
		{   /* group previous elements into a list, if more than one: */
			if (_prev_parts == 0)
				;
			else if (_prev_parts->prev == 0)
				alt->result = _prev_parts->last;
			else
			{   alt->result.createList();
				while (_prev_parts != 0)
				{   alt->result.insertChild(_prev_parts->last);
					_prev_parts = _prev_parts->prev;
				}
			}
		}
		//DEBUG_EXIT("parse_rule = ");
		//DEBUG_PT(_rtree); DEBUG_NL;
		//alt->succeed();
		alt->finishCall();
		alt->execute(parser);
		return;
	}

#ifdef DEBUG
	if (detailed_debug)
	{
		printf("  at ");
		{	for (ParseProcess* parseProcess = this; parseProcess != 0; parseProcess = parseProcess->parent())
				if (parseProcess->printNT(stdout))
					break;
		}
		printf(": ");
		_rule->print(stdout);
		printf("\n");
	}
#endif
	parser->_current_rule = _rule;
	_optional = _rule->optional;
	_nongreedy = _rule->nongreedy;
	_avoid = _rule->avoid;
	_sequential = _rule->sequential;
	_chain_sym = _rule->chain_symbol;

	if (_optional)
	{
		if (_avoid)
		{
			Alternative* child_alt_missing = alt->addChild(11);
			child_alt_missing->cr_text = "Rule_miss_avoid";
			child_alt_missing->cr_grammar_rule = _rule;
			parser->insert(child_alt_missing);
		}

		Alternative* child_alt_present = alt->addChild(1);
		child_alt_present->cr_text = "Rule_pres";
		child_alt_present->cr_grammar_rule = _rule;
		parser->insert(child_alt_present);

		if (!_avoid)
		{
			Alternative* child_alt_missing = alt->addChild(11);
			child_alt_missing->cr_text = "Rule_miss";
			child_alt_missing->cr_grammar_rule = _rule;
			parser->insert(child_alt_missing);
		}

		alt->releaseProcess();
		return;
	}

state1:
	DEBUG_ENTER("Rule:try");
	_try_it = false;

	parser->_current_rule = _rule;

	/* Try to accept first symbol */
   	_start_pos = parser->_text;
	_is_terminal = false;

	switch( _rule->kind )
	{   case RK_T_EOF:
        	_try_it = parser->_scanner->acceptEOF(parser->_text);
			if (!_try_it)
				parser->expected_string("eof", false);
        	if (_try_it)
        	{
				alt->call(new ParseRuleProcess(_rule->next, _prev_parts, _tree_name));
				parser->insert(alt);
				_state = 2; return; state2:
				DEBUG_ENTER("Rule:eof");
				if (_optional)
					alt->succeed();
				alt->finishCall();
				alt->execute(parser);
				return;
			}
			break;
		case RK_TERM:
            _try_it = parser->parse_term(_rule->text.terminal, _t);
            _is_terminal = true;
            break;
		case RK_IDENT:
            _try_it = parser->parse_ident(_rule->text.ident, _t);
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
			alt->call(new ParseNTProcess(_rule->text.non_terminal));
			parser->insert(alt);
			_state = 3; return; state3:
			DEBUG_ENTER("Rule:done_NT");
            _try_it = true;
			_t = alt->result;
			break;
        case RK_LIT:
        case RK_WS_TERM:
        case RK_WS_NT:
		{   if (_rule->kind == RK_LIT)
				_try_it = parser->_scanner->acceptLiteral(parser->_text, _rule->str_value);
			else if (_rule->kind == RK_WS_TERM)
				_try_it = parser->parse_ws_term(_rule->text.terminal);
			else
			{
				alt->call(new ParseNTProcess(_rule->text.non_terminal));
				parser->insert(alt);
				_state = 4; return; state4:
				DEBUG_ENTER("Rule:done_NT (for WS_NT)");
				_try_it = true;
				_t = alt->result;
			}
			if (_try_it)
			{   
				//printf("%d.%d: acceptLiteral(%s) : success\n", _start_pos.line(), _start_pos.column(), _rule->str_value);
				if (_sequential)
				{   
					_seq_val.prev = 0;

					alt->call(new ParseSeqProcess(_rule, _chain_sym, &_seq_val, _prev_parts, _tree_name));
					parser->insert(alt);
					_state = 5; return; state5:
					DEBUG_ENTER("Rule:done_Lit_SEQ");
					alt->result.setLineColumn(_start_pos.line(), _start_pos.column());
					if (_optional)
						alt->succeed();
					alt->finishCall();
					alt->execute(parser);
					return;
				}
				else
				{
					alt->call(new ParseRuleProcess(_rule->next, _prev_parts, _tree_name));
					parser->insert(alt);
					_state = 6; return; state6:
					DEBUG_ENTER("Rule:done_Lit");
					/* use for debugging setLineColumn problem
					alt->print(stdout, 0);
					printf("|");
					*/
					alt->result.setLineColumn(_start_pos.line(), _start_pos.column());
					if (_optional)
						alt->succeed();
					alt->finishCall();
					alt->execute(parser);
					return;
				}
			}
			else
			{
		    	//printf("%d.%d: acceptLiteral(%s) failed at '%s'\n", _start_pos.line(), _start_pos.column(), _rule->str_value, parser->_text.start());
			}
			alt->fail();
			return;
		}
		case RK_CHARSET:
			_try_it = _rule->text.char_set->contains_char(*parser->_text);
			if (!_try_it)
				parser->expected_string("<charset>", false);
			else
			{	_t.createCharAtom(*parser->_text);
				parser->_text.next();
				parser->_scanner->skipSpace(parser->_text);
			}
			break;
		case RK_AVOID:
			_try_it = !_rule->text.char_set->contains_char(*parser->_text);
			break;
		case RK_COLOURCODING:
			_try_it = true;
			break;
		case RK_OR_RULE:
			alt->call(new ParseOrRuleProcess(_rule->text.or_rules->first, (ParsedValue*)0));
			parser->insert(alt);
			_state = 7; return; state7:
			DEBUG_ENTER("Rule:done_or");
			_try_it = true;
			_t = alt->result;
			break;
		case RK_COR_RULE:
			alt->call(new ParseOrRuleProcess(_rule->text.or_rules->first, _prev_parts));
			parser->insert(alt);
			_state = 8; return; state8:
			DEBUG_ENTER("Rule:done_cor");
			alt->finishCall();
			alt->execute(parser);
			return;
			break;
		default:
			_try_it = false;
	}

	if (!_try_it)
	{
		alt->fail();
		return;
	}

	/* We succeded in parsing the first element */
	if (_optional && !_nongreedy)
		alt->succeed();

	if (!_t.isEmpty() && _t.line() == 0)
	    _t.setLineColumn(_start_pos.line(), _start_pos.column());

	if (_sequential)
	{   _seq_val.last = _t;
		_seq_val.prev = 0;
		
		alt->call(new ParseSeqProcess(_rule, _chain_sym, &_seq_val, _prev_parts, _tree_name));
		parser->insert(alt);
		_state = 9; return; state9:
		DEBUG_ENTER("Rule:done_Seq");
		if (_is_terminal)
            alt->result.setLineColumn(_start_pos.line(), _start_pos.column());
		if (_optional && _nongreedy)
			alt->succeed();
		alt->finishCall();
		alt->execute(parser);
		return;
	}
	else
	{
		_val.last = _t;
		_val.prev = _prev_parts;

		alt->call(new ParseRuleProcess(_rule->next, &_val, _tree_name));
		parser->insert(alt);
		_state = 10; return; state10:
		DEBUG_ENTER("Rule:done_rule");
#ifdef DEBUG
		if (detailed_debug)
		{	printf("  at ");
			{	for (ParseProcess* parseProcess = this; parseProcess != 0; parseProcess = parseProcess->parent())
					if (parseProcess->printNT(stdout))
						break;
			}
			printf(": ");
			_rule->print(stdout);
			printf("\n");
		}
#endif
		if (_is_terminal)
            alt->result.setLineColumn(_start_pos.line(), _start_pos.column());
		if (_optional && _nongreedy)
			alt->succeed();
		alt->finishCall();
		alt->execute(parser);
		return;
	}

state11:
	/* First element was optional: */
	DEBUG_ENTER("Rule:Opt");
	_val.prev = _prev_parts;

	alt->call(new ParseRuleProcess(_rule->next, _rule->kind == RK_LIT ? _prev_parts : &_val, _tree_name));
	parser->insert(alt);
	_state = 12; return; state12:
	DEBUG_ENTER("Rule:done_Opt");
	alt->succeed();
	alt->finishCall();
	alt->execute(parser);
}

void ParseRuleProcess::print(FILE* fout)
{
	//if (_parent_process != 0)
	//	_parent_process->print(fout);
	fprintf(fout, " rule:%d ", _state);
	if (_rule != 0)
		_rule->print(fout);		
		//fprintf(fout, "[%d.%d]", _rule->line, _rule->column);
	else
		fprintf(fout, "![%s]", _tree_name.val());
}

void ParseSeqProcess::execute(ParParser* parser, Alternative* alt)
{   
	switch(_state) {
		case 1: goto state1;
		case 2: goto state2;
		case 3: goto state3;
		case 4: goto state4;
		case 5: goto state5;
		case 6: goto state6;
		case 7: goto state7;
	}

	DEBUG_ENTER("Seq:init");
	{
		if (_rule->avoid)
		{
			Alternative* child_alt_over = alt->addChild(6);
			child_alt_over->cr_text = "Seq_over_avoid";
			child_alt_over->cr_grammar_rule = _rule;
			parser->insert(child_alt_over);
		}

		Alternative* child_alt_more = alt->addChild(1);
		child_alt_more->cr_text = "Seq_more";
		child_alt_more->cr_grammar_rule = _rule;
		parser->insert(child_alt_more);

		if (!_rule->avoid)
		{
			Alternative* child_alt_over = alt->addChild(6);
			child_alt_over->cr_text = "Seq_over_normal";
			child_alt_over->cr_grammar_rule = _rule;
			parser->insert(child_alt_over);
		}

		alt->releaseProcess();
		return;
	}	
	
state1:
	DEBUG_ENTER("Seq:try");
	parser->_current_rule = _rule;

	/* Try to accept chain symbol */
	if (_chain_sym != 0 && *_chain_sym != '\0')
	{
		if (!parser->_scanner->acceptLiteral(parser->_text, _chain_sym))
		{
			alt->fail();
			return;
		}
		parser->insert(alt);
		_state = 2; return; state2:
		DEBUG_ENTER("Seq:done_Lit");
	}

#ifdef DEBUG
	if (detailed_debug)
	{
		printf("  at  ");
		{	for (ParseProcess* parseProcess = this; parseProcess != 0; parseProcess = parseProcess->parent())
				if (parseProcess->printNT(stdout))
					break;
		}
		printf(": ");
		_rule->print(stdout);
		printf("\n");
	}
#endif
	parser->_current_rule = _rule;

	/* Try to accept first symbol */
	_start_pos = parser->_text;
	_try_it = false;
	switch( _rule->kind )
	{   case RK_T_EOF:
			_try_it = parser->_scanner->acceptEOF(parser->_text);
			if (_try_it)
			   _t = Ident("EOF").val();
			break;
		case RK_TERM:
            _try_it = parser->parse_term(_rule->text.terminal, _t);
            break;
		case RK_WS_TERM:
            _try_it = parser->parse_ws_term(_rule->text.terminal);
            break;
		case RK_IDENT:
            _try_it = parser->parse_ident(_rule->text.ident, _t);
            break;
		case RK_NT:
		case RK_WS_NT:
			alt->call(new ParseNTProcess(_rule->text.non_terminal));
			parser->insert(alt);
			_state = 3; return; state3:
			DEBUG_ENTER("Seq:done_NT");
			_try_it = true;
			if (_rule->kind == RK_NT)
				_t = alt->result;
			break;
		case RK_LIT:
            _try_it = parser->_scanner->acceptLiteral(parser->_text, _rule->str_value);
            if (!_try_it)
            {	
#ifdef DEBUG
				if (detailed_debug)
					printf("%d.%d: acceptLiteral(%s) : success\n", _start_pos.line(), _start_pos.column(), _rule->str_value.val());
#endif
				parser->expected_string(_rule->str_value, true);
			}
			else
			{
#ifdef DEBUG
				if (detailed_debug)
		    		printf("%d.%d: acceptLiteral(%s) failed at '%s'\n", _start_pos.line(), _start_pos.column(), _rule->str_value.val(), parser->_text.start());
#endif
			}
			break;
		case RK_CHARSET:
			_try_it = _rule->text.char_set->contains_char(*parser->_text);
			if (!_try_it)
				parser->expected_string("<charset>", false);
			else
			{	_t.createCharAtom(*parser->_text);
				parser->_text.next();
				parser->_scanner->skipSpace(parser->_text);
			}
			break;
		case RK_AVOID:
			_try_it = !_rule->text.char_set->contains_char(*parser->_text);
			break;
		case RK_COLOURCODING:
			_try_it = true;
			break;
		case RK_OR_RULE:
			alt->call(new ParseOrRuleProcess(_rule->text.or_rules->first, (ParsedValue*)0));
			parser->insert(alt);
			_state = 4; return; state4:
			DEBUG_ENTER("Seq:done_Or");
			_try_it = true;
			_t = alt->result;
			break;
		default:
			_try_it = false;
	} 

	if (!_try_it)
	{
		alt->fail();
		return;
	}

	/* We succeded in parsing the first element */
	if (!_t.isEmpty() && _t.line() == 0)
	    _t.setLineColumn(_start_pos.line(), _start_pos.column());

	_val.last.attach(_t);
    _val.prev = _prev_seq_parts;

	alt->call(new ParseSeqProcess(_rule, _chain_sym, &_val, _prev_parts, _tree_name));
	parser->insert(alt);
	_state = 5; return; state5:
	DEBUG_ENTER("Seq:try_finish");
	alt->succeed();
	alt->finishCall();
	alt->execute(parser);
	return;

state6:
	DEBUG_ENTER("Seq:end");
	{
		AbstractParseTree seq;
		seq.createList();
		for (ParsedValue* prev_seq_parts = _prev_seq_parts; prev_seq_parts != 0; prev_seq_parts = prev_seq_parts->prev)
			seq.insertChild(prev_seq_parts->last);
		_val.last.attach(seq);
		_val.prev = _prev_parts;
	}
	alt->call(new ParseRuleProcess(_rule->next, &_val, _tree_name));
	parser->insert(alt);
	_state = 7; return; state7:
	DEBUG_ENTER("Seq:end_finish");
	alt->succeed();
	alt->finishCall();
	alt->execute(parser);
}

void ParseSeqProcess::print(FILE* fout)
{
	if (_parent_process != 0)
		_parent_process->print(fout);
	fprintf(fout, " seq:%d", _state);
}


void ParParser::expected_string(const char *s, bool is_keyword)
{
	AbstractParser::expected_string(_text, s, is_keyword);
}

void ParParser::insert(Alternative *alt)
{
	ParsePosition **ref_parse_pos = &_parse_position;
	while ((*ref_parse_pos) != 0 && (*ref_parse_pos)->filePos() < _text)
		ref_parse_pos = &(*ref_parse_pos)->next;

	if ((*ref_parse_pos) == 0 || (*ref_parse_pos)->filePos() > _text)
		*ref_parse_pos = new ParsePosition(*ref_parse_pos, _text);

	(*ref_parse_pos)->append(alt);
	if (this->_debug_parse)
	{
		printf(" insert: ");
		alt->print(stdout);
		printf("\n");
	}
}

void ParParser::check(Alternative* root_alt)
{

	// Mark all active alternatives
	{
	for (ParsePosition* parse_position = _parse_position; parse_position != 0; parse_position = parse_position->next)
	{
		for (Alternative* alternative = parse_position->alternatives; alternative != 0; alternative = alternative->_next)
			alternative->_check_state = 'A';
	}
	}
	root_alt->check();

	// Mark all active alternatives
	for (ParsePosition* parse_position = _parse_position; parse_position != 0; parse_position = parse_position->next)
	{
		for (Alternative* alternative = parse_position->alternatives; alternative != 0; alternative = alternative->_next)
		{
			if (alternative->_check_state != 'F')
			{
				printf("ERROR12");
				exit(1);
			}
			alternative->_check_state = ' ';
		}
	}
}

bool ParParser::parse(const TextFileBuffer& textBuffer, Ident root_id, AbstractParseTree& rtree)
{
	_text = textBuffer;
	_f_file_pos = _text;
	_nr_exp_syms = 0;
	int count = 0;
	
	_result = false;
	detailed_debug = true;

	_scanner->initScanning(this);
	_scanner->skipSpace(_text);
	Alternative* root_alt = new Alternative(0);
	Alternative* alt = new Alternative(root_alt);
	alt->call(new ParseRootProcess(root_id));
	_parse_position = 0;
	insert(alt);
	for(;;)
	{
		count++;
		/*printf("%d ", count);
		if (count > 25000)
		{
			_debug_parse = true;
			_debug_scan = true;

			detailed_debug = true;
		}*/
#ifdef TREE_CHECK
		check(root_alt);
#endif
		if (_debug_parse)
			root_alt->print(stdout,0);
		while (_parse_position != 0 && _parse_position->alternatives == 0)
		{
			ParsePosition *next = _parse_position->next;
			delete _parse_position;
			_parse_position = next;
		}
		if (_parse_position == 0)
			break;
		
		Alternative* alt = _parse_position->alternatives;
		if (_debug_parse)
		{
			printf("\nExecute: ");
			alt->print(stdout);
			printf("\n");
		}
		alt->remove();
		_text = _parse_position->filePos();
		alt->execute(this);
	}

	if (_result)
		rtree.attach(_result_tree);

#ifdef DEBUG
	if (detailed_debug)
		alt->print(stdout, 0);

#endif

#ifdef DEBUG_ALLOC_ALT
	ParsePosition::allocs.print();
	Alternative::allocs.print();
	Alternative::print_live();
	ParseNTProcess::allocs.print();
	ParseOrRuleProcess::allocs.print();
	ParseRuleProcess::allocs.print();
	ParseSeqProcess::allocs.print();
#endif

	return _result;
}

#undef DEBUG_ENTER
//#undef DEBUG_ENTER_P1
//#undef DEBUG_EXIT
//#undef DEBUG_EXIT_P1
//#undef DEBUG_TAB
//#undef DEBUG_NL
//#undef DEBUG_PT
//#undef DEBUG_PO
//#undef DEBUG_PR
//#undef DEBUG_
//#undef DEBUG_P1
