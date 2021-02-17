#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "Ident.h"
#include "String.h"
#include "AbstractParseTree.h"
#include "Unparser.h"

void BasicTerminalUnparser::init(AbstractStream<char> *outputStream)
{
	_stream.setOutputStream(outputStream);
	_indent = 0;
	_state = '\0';
}

bool BasicTerminalUnparser::match(Ident terminal, const AbstractParseTree& tree)
{
	static Ident id_ident = "ident";
	static Ident id_string = "string";
	static Ident id_cstring = "cstring";
	static Ident id_char = "char";
	static Ident id_int = "int";
	static Ident id_double = "double";

	if (terminal == id_ident)
		return tree.isIdent();
	if (terminal == id_string || terminal == id_cstring)
		return tree.isString();
	if (terminal == id_char)
		return tree.isChar();
	if (terminal == id_int)
		return tree.isInt();
	if (terminal == id_double)
		return tree.isDouble();

	assert(0);
	return false;
}

void BasicTerminalUnparser::unparse(Ident terminal, const AbstractParseTree& tree)
{
	static Ident id_ident = "ident";
	static Ident id_string = "string";
	static Ident id_cstring = "cstring";
	static Ident id_char = "char";
	static Ident id_int = "int";
	static Ident id_double = "double";

	if (_stream.col() == 1)
	{
		for (int i = 0; i < _indent; i++)
			_stream.emit(' ');
		_state = ' ';
	}

	if (terminal == id_ident)
	{
		if (tree.isIdent())
		{
			if (_state == 'a')
				_stream.emit(' ');
			for (const char *s = tree.identName().val(); *s != '\0'; s++)
				_stream.emit(*s);
			_state = 'a';
		}
		else
			assert(0);
	}
	else if (terminal == id_string || terminal == id_cstring)
	{
		if (tree.isString())
		{
			if (_state == 'a')
				_stream.emit(' ');
			_stream.emit('"');
			for (const char *s = tree.stringValue(); *s != '\0'; s++)
			{
				if (*s == '\"' || *s == '\\')
				{
					_stream.emit('\\');
					_stream.emit(*s);
				}
				else if (*s == '\n')
				{
					_stream.emit('\\');
					_stream.emit('n');
				}
				else if (*s == '\r')
				{
					_stream.emit('\\');
					_stream.emit('r');
				}
				else if (*s == '\t')
				{
					_stream.emit('\\');
					_stream.emit('t');
				}
				else if (*s == '\0')
				{
					_stream.emit('\\');
					_stream.emit('0');
				}
				else if ((unsigned char)*s >= ' ')
					_stream.emit(*s);
			}
			_stream.emit('"');
			_state = '"';
		}
		else
			assert(0);
	}
	else if (terminal == id_char)
	{
		if (tree.isChar())
		{
			if (_state == 'a')
				_stream.emit(' ');
			_stream.emit('\'');
			char ch = tree.charValue();
			if (ch == '\'' || ch == '\\')
			{
				_stream.emit('\\');
				_stream.emit(ch);
			}
			else if (ch == '\n')
			{
				_stream.emit('\\');
				_stream.emit('n');
			}
			else if (ch == '\r')
			{
				_stream.emit('\\');
				_stream.emit('r');
			}
			else if (ch == '\t')
			{
				_stream.emit('\\');
				_stream.emit('t');
			}
			else if (ch == '\0')
			{
				_stream.emit('\\');
				_stream.emit('0');
			}
			else if (ch >= ' ')
			{
				_stream.emit(ch);
			}
			_stream.emit('\'');
			_state = '"';
		}
		else
			assert(0);
	}
	else if (terminal == id_int)
	{
		if (tree.isInt())
		{
			if (_state == 'a')
				_stream.emit(' ');
			char buffer[20];
			sprintf(buffer, "%ld", tree.intValue());
			for (const char *s = buffer; *s != '\0'; s++)
				_stream.emit(*s);
			_state = '1';
		}
		else
			assert(0);
	}
	else if (terminal == id_double)
	{
		if (tree.isDouble())
		{
			if (_state == 'a')
				_stream.emit(' ');
			char buffer[100];
			sprintf(buffer, "%lf", tree.doubleValue());
			// remove trailing zero's
			for (char *s = buffer + strlen(buffer) - 1; s > buffer; s--)
				if (*s == '0')
					*s = '\0';
				else
					break;
			for (const char *s = buffer; *s != '\0'; s++)
				_stream.emit(*s);
			_state = '1';
		}
		else
			assert(0);
	}
	else
		assert(0);
}

void BasicTerminalUnparser::unparseLiteral(const char* literal)
{
	if (_stream.col() == 1)
	{
		for (int i = 0; i < _indent; i++)
			_stream.emit(' ');
		_state = ' ';
	}

	if (isalnum(*literal) && _state == 'a')
		_stream.emit(' ');
	
	unsigned char last_ch;
	for (const char *s = literal; *s != '\0'; s++)
	{
		last_ch = *s; 
		_stream.emit(*s);
	}
	
	if (last_ch == ',' && literal[1] == '\0')
	{
		_stream.emit(' ');
		_state = ' ';
	}
	else
		_state = isalnum(last_ch) ? 'a' : ',';

}

void BasicTerminalUnparser::unparseWhiteSpace(Ident terminal)
{
	static Ident id_inc = "inc";
	static Ident id_dec = "dec";
	static Ident id_nl = "nl";
	static Ident id_s = "s";

	if (terminal == id_inc)
		_indent += 4;
	else if (terminal == id_dec)
		_indent -= 4;
	else if (terminal == id_nl)
		_stream.emit('\n');
	else if (terminal == id_s)
	{
		if (_stream.col() == 1)
			for (int i = 0; i < _indent; i++)
				_stream.emit(' ');
		else if (_state != ' ')
			_stream.emit(' ');
		_state = ' ';
	}
}

bool WhiteSpaceTerminalUnparser::match(Ident terminal, const AbstractParseTree& tree)
{
	static Ident id_ws = "ws";

	if (terminal == id_ws)
		return tree.isString();
	return BasicTerminalUnparser::match(terminal, tree);
}

void WhiteSpaceTerminalUnparser::unparse(Ident terminal, const AbstractParseTree& tree)
{
	static Ident id_ws = "ws";

	if (terminal == id_ws)
	{
		for (const char *s = tree.stringValue(); *s != '\0'; s++)
			_stream.emit(*s);
	}
	else
	{
		_state = ' '; // to prevent unparser for emit additional white space
		BasicTerminalUnparser::unparse(terminal, tree);
	}
}

void WhiteSpaceTerminalUnparser::unparseWhiteSpace(Ident terminal)
{
	// do nothing
}

void WhiteSpaceTerminalUnparser::unparseLiteral(const char* literal)
{
	for (const char *s = literal; *s != '\0'; s++)
		_stream.emit(*s);
}


void ResourceTerminalUnparser::init(AbstractStream<char> *outputStream)
{
	_stream.setOutputStream(outputStream);
	_indent = 0;
	_state = '\0';
}

bool ResourceTerminalUnparser::match(Ident terminal, const AbstractParseTree& tree)
{
	static Ident id_comment = "comment";
	static Ident id_ident = "ident";
	static Ident id_string = "string";
	static Ident id_int = "int";
	static Ident id_hexint = "hexint";
	static Ident id_hexintL = "hexintL";

	if (terminal == id_comment)
		return tree.isString();
	if (terminal == id_ident)
		return tree.isIdent();
	if (terminal == id_string)
		return tree.isString();
	if (terminal == id_int || terminal == id_hexint || terminal == id_hexintL)
		return tree.isInt();

	assert(0);
	return false;
}

void ResourceTerminalUnparser::unparse(Ident terminal, const AbstractParseTree& tree)
{
	static Ident id_comment = "comment";
	static Ident id_ident = "ident";
	static Ident id_string = "string";
	static Ident id_int = "int";
	static Ident id_hexint = "hexint";
	static Ident id_hexintL = "hexintL";

	if (_stream.col() == 1)
	{
		for (int i = 0; i < _indent; i++)
			_stream.emit(' ');
		_state = ' ';
	}

	if (terminal == id_comment)
	{
		if (tree.isString())
		{
			if (_state == 'a')
				_stream.emit(' ');
			_stream.emit('/');
			_stream.emit('/');
			for (const char *s = tree.stringValue(); *s != '\0'; s++)
				_stream.emit(*s);
			_state = '/';
		}
		else
			assert(0);
	}
	else if (terminal == id_ident)
	{
		if (tree.isIdent())
		{
			if (_state == 'a')
				_stream.emit(' ');
			for (const char *s = tree.identName().val(); *s != '\0'; s++)
				_stream.emit(*s);
			_state = 'a';
		}
		else
			assert(0);
	}
	else if (terminal == id_string)
	{
		if (tree.isString())
		{
			if (_state == 'a')
				_stream.emit(' ');
			_stream.emit('"');
			for (const char *s = tree.stringValue(); *s != '\0'; s++)
			{
				_stream.emit(*s);
				if (*s == '"')
					_stream.emit('"');
			}
			_stream.emit('"');
			_state = '"';
		}
		else
			assert(0);
	}
	else if (terminal == id_int)
	{
		if (tree.isInt())
		{
			if (_state == 'a')
				_stream.emit(' ');
			char buffer[20];
			sprintf(buffer, "%ld", tree.intValue());
			for (const char *s = buffer; *s != '\0'; s++)
				_stream.emit(*s);
			_state = '1';
		}
		else
			assert(0);
	}
	else if (terminal == id_hexint || terminal == id_hexintL)
	{
		if (tree.isInt())
		{
			if (_state == 'a')
				_stream.emit(' ');
			char buffer[20];
			sprintf(buffer, "0x%lx", tree.intValue());
			for (const char *s = buffer; *s != '\0'; s++)
				_stream.emit(*s);
			if (terminal == id_hexintL)
				_stream.emit('L');
			_state = '1';
		}
		else
			assert(0);
	}
	else
		assert(0);
}

void ResourceTerminalUnparser::unparseLiteral(const char* literal)
{
	if (literal[0] == '\n' && literal[1] == '\0')
	{
		_stream.emit('\n');
		return;
	}

	if (_stream.col() == 1)
	{
		for (int i = 0; i < _indent; i++)
			_stream.emit(' ');
		_state = ' ';
	}

	if (isalnum((unsigned char)literal[0]) || literal[0] == '#')
	{
		if (_state == 'a')
			_stream.emit(' ');
		_state = 'a';
	}
	else
		_state = ',';

	if (literal[0] == '|' && literal[1] == '\0')
	{
		if (_state != ' ')
			_stream.emit(' ');
		_stream.emit('|');
		_stream.emit(' ');
		_state = ' ';
		return;
	}

	for (const char *s = literal; *s != '\0'; s++)
		_stream.emit(*s);
}

void ResourceTerminalUnparser::unparseWhiteSpace(Ident terminal)
{
	static Ident id_s = "s";
	static Ident id_s1 = "s1";
	static Ident id_cm = "cm";
	static Ident id_inc = "inc";
	static Ident id_dec = "dec";
	static Ident id_cnl = "cnl";
	static Ident id_stnl = "stnl";
	static Ident id_t3 = "t3";
	static Ident id_t4 = "t4";
	static Ident id_t5 = "t5";
	static Ident id_t6 = "t6";
	static Ident id_t8 = "t8";
	static Ident id_t10 = "t10";
	static Ident id_t12 = "t12";

	if (terminal == id_s)
	{
		if (_state != ' ')
			_stream.emit(' ');
		_state = ' ';
	}
	else if (terminal == id_s1)
	{
		_stream.emit(' ');
		_state = ' ';
	}
	else if (terminal == id_cm)
	{
		_stream.emit(',');
		_state = ',';
	}
	else if (terminal == id_inc)
		_indent += 4;
	else if (terminal == id_dec)
		_indent -= 4;
	else if (terminal == id_cnl)
	{
		if (_stream.col() >= 73)
		{
			_stream.emit('\n');
			while (_stream.col() <= 20)
				_stream.emit(' ');
			_state = ' ';
		}
	}
	else if (terminal == id_stnl)
	{
		if (_stream.col() > 29)
			_stream.emit('\n');
		while (_stream.col() < 29)
			_stream.emit(' ');
		_state = ' ';
	}
	else if (terminal == id_t3)
	{
		while (_stream.col() <= 12 + _indent)
			_stream.emit(' ');
		_state = ' ';
	}
	else if (terminal == id_t4)
	{
		while (_stream.col() <= 16 + _indent)
			_stream.emit(' ');
		_state = ' ';
	}
	else if (terminal == id_t5)
	{
		while (_stream.col() <= 20 + _indent)
			_stream.emit(' ');
		_state = ' ';
	}
	else if (terminal == id_t6)
	{
		while (_stream.col() <= 24 + _indent)
			_stream.emit(' ');
		_state = ' ';
	}
	else if (terminal == id_t8)
	{
		while (_stream.col() <= 32 + _indent)
			_stream.emit(' ');
		_state = ' ';
	}
	else if (terminal == id_t10)
	{
		while (_stream.col() <= 40 + _indent)
			_stream.emit(' ');
		_state = ' ';
	}
	else if (terminal == id_t12)
	{
		while (_stream.col() <= 48 + _indent)
			_stream.emit(' ');
		_state = ' ';
	}
}

bool MarkDownCTerminalUnparser::match(Ident terminal, const AbstractParseTree& tree)
{
	static Ident id_macro_ident = "macro_ident";
	static Ident id_macro_def = "macro_def";
	
	if (terminal == id_macro_ident)
		return tree.isIdent();
	if (terminal == id_macro_def)
		return tree.isString();
	
	return BasicTerminalUnparser::match(terminal, tree);
}

void MarkDownCTerminalUnparser::unparse(Ident terminal, const AbstractParseTree& tree)
{
	static Ident id_macro_ident = "macro_ident";
	static Ident id_macro_def = "macro_def";
	static Ident id_ident = "ident";
	
	if (terminal == id_macro_ident)
	{
		BasicTerminalUnparser::unparse(id_ident, tree);
		return;
	}
	if (terminal == id_macro_def)
	{
		_stream.emit(' ');
		for (const char *s = tree.stringValue(); *s != '\0'; s++)
		{
			_stream.emit(*s);
		}
		return;
	}
	
	BasicTerminalUnparser::unparse(terminal, tree);
}


bool Unparser::match_or(const AbstractParseTree& tree, GrammarOrRules* or_rules, bool nt_with_recursive/*= false*/)
{
	if (!nt_with_recursive && or_rules->first != 0 && or_rules->first->tree_name.empty() && or_rules->first->next == 0)
	{
		GrammarRule* rule = or_rules->first->rule;
		if (or_rules->first->single_element != 0)
			return match_rule_single(tree, rule);
		else if (tree.isList() || tree.isTree())
			return match_rule(tree, rule);
	}

	TreeTypeToGrammarRules* treeTypeToRules = 0;
	bool is_single = false;
	if (tree.isEmpty())
	{
		if (or_rules->emptyTreeRules != 0)
			return true;
	}
	else if (tree.isTree())
	{
		for (TreeTypeToGrammarRules* treeNameToRules = or_rules->treeNameToRulesMap; treeNameToRules != 0; treeNameToRules = treeNameToRules->next)
		{
			if (tree.isTree(treeNameToRules->name))
			{
				treeTypeToRules = treeNameToRules;
				break;
			}
		}
	}
	else if (tree.isList())
		treeTypeToRules = or_rules->listRules;
	else
	{
		for (TreeTypeToGrammarRules* terminalToRules = or_rules->terminalToRulesMap; terminalToRules != 0; terminalToRules = terminalToRules->next)
		{
			if (_terminalUnparser->match(terminalToRules->name, tree))
				return true;
		}
	}

	if (treeTypeToRules == 0)
		return false;

	for (TreeTypeToGrammarRule *treeTypeToRule = treeTypeToRules->alternatives; treeTypeToRule != 0; treeTypeToRule = treeTypeToRule->next)
		if (match(tree, treeTypeToRule))
			return true;

	return false;
}

class Indent
{
public:
	Indent(const char *name) : _name(name), _result(false)
	{
		newline();
		fprintf(stdout, "Enter: %s", _name);
		_depth += 2;
	}
	~Indent()
	{
		if (_result) return;
		_depth -= 2;
		newline();
		fprintf(stdout, "Leave: %s", _name);
	}
	bool result(bool value)
	{
		_depth -= 2;
		newline();
		fprintf(stdout, "Leave: %s = %s", _name, value ? "true" : "false");
		_result = true;
		return value;
	}
	void newline() { fprintf(stdout, "\n%*.*s", _depth, _depth, ""); }
private:
	const char *_name;
	bool _result;
	static int _depth;
};
int Indent::_depth = 0;
#define DEBUG_ENTER(N) //Indent indent(N);
#define DEBUG_ARG_APT(P) //indent.newline(); fprintf(stdout, "- "); P.print(stdout, true);
#define DEBUG_ARG_G(G) //indent.newline(); fprintf(stdout, "- "); if (G != 0) G->print(stdout);
#define DEBUG_ARG_NG(N, G) //indent.newline(); fprintf(stdout, "- %s ", N); if (G != 0) G->print(stdout);
#define DEBUG_RESULT(V) V //indent.result(V)

bool Unparser::match_rule_elem(const AbstractParseTree& part, GrammarRule* rule)
{
	DEBUG_ENTER("match_rule_elem");
	DEBUG_ARG_APT(part);
	DEBUG_ARG_G(rule);

	switch (rule->kind)
	{
		case RK_T_EOF:
			// Done
			return DEBUG_RESULT(true);
			break;
		case RK_TERM:
			return DEBUG_RESULT(_terminalUnparser->match(rule->text.terminal->name, part));
			break;
		case RK_WS_TERM:
			return DEBUG_RESULT(true);
			break;
		case RK_IDENT:
			return true;
			break;
		case RK_NT:
			return DEBUG_RESULT(match_or(part, rule->text.non_terminal, rule->text.non_terminal->recursive != 0));
			break;
		case RK_WS_NT:
			return DEBUG_RESULT(true);
			break;
		case RK_LIT:
			return DEBUG_RESULT(true);
			break;
		case RK_OR_RULE:
			return DEBUG_RESULT(match_or(part, rule->text.or_rules));
			break;
	}
	return DEBUG_RESULT(false);
}

bool Unparser::match_rule(AbstractParseTree::iterator partIt, GrammarRule* rule)
{
	DEBUG_ENTER("match_rule");
	DEBUG_ARG_G(rule);

	for (; rule != 0; rule = rule->next)
	{
		if (rule->kind == RK_TERM || rule->kind == RK_NT || rule->kind == RK_OR_RULE)
		{
			AbstractParseTree part;
			if (partIt.more())
			{
				part = AbstractParseTree(partIt);
				partIt.next();
			}

			if (rule->optional && part.isEmpty())
				;
			else if (rule->sequential && part.isList())
			{
				for (AbstractParseTree::iterator elemIt(part); elemIt.more(); elemIt.next())
				{
					AbstractParseTree elem(elemIt);
					if (!match_rule_elem(elem, rule))
						return DEBUG_RESULT(false);
				}
			}
			else
			{
				if (!match_rule_elem(part, rule))
					return DEBUG_RESULT(false);
			}
		}
		/*
		else
		{
			AbstractParseTree empty_tree;
			if (!match_rule_elem(empty_tree, rule))
				return false;
		}
		*/
	}

	return DEBUG_RESULT(true);
}

bool Unparser::match_rule_single(const AbstractParseTree& tree, GrammarRule* rule)
{
	DEBUG_ENTER("match_rule_single");
	DEBUG_ARG_APT(tree);
	DEBUG_ARG_G(rule);

	for (; rule != 0; rule = rule->next)
	{
		if (rule->kind == RK_TERM || rule->kind == RK_NT || rule->kind == RK_OR_RULE)
		{
			if (rule->optional && tree.isEmpty())
				;
			else if (rule->sequential && tree.isList())
			{
				for (AbstractParseTree::iterator elemIt(tree); elemIt.more(); elemIt.next())
				{
					AbstractParseTree elem(elemIt);
					if (!match_rule_elem(elem, rule))
						return DEBUG_RESULT(false);
				}
			}
			else
			{
				if (!match_rule_elem(tree, rule))
					return DEBUG_RESULT(false);
			}
			return true;
		}
		/*else
		{
			AbstractParseTree empty_tree;
			if (!match_rule_elem(empty_tree, rule))
				return false;
		}*/
	}
	return DEBUG_RESULT(true);
}

bool Unparser::match(const AbstractParseTree& tree, TreeTypeToGrammarRule *treeTypeToRule)
{
	DEBUG_ENTER("match");
	DEBUG_ARG_APT(tree);
	DEBUG_ARG_NG("top", treeTypeToRule->top_rule);
	DEBUG_ARG_G(treeTypeToRule->rule);

	if (treeTypeToRule->top_rule != 0)
		return match_rule_single(tree, treeTypeToRule->top_rule);
	else if (treeTypeToRule->rec_nt != 0)
	{
		AbstractParseTree::iterator partIt(tree);
		if (!match_or(partIt, treeTypeToRule->rec_nt, treeTypeToRule->rec_nt->recursive != 0))
			return DEBUG_RESULT(false);
		partIt.next();
		return DEBUG_RESULT(match_rule(partIt, treeTypeToRule->rule));
	}
	else if (treeTypeToRule->is_single)
		return DEBUG_RESULT(match_rule_single(tree, treeTypeToRule->rule));
	else
		return DEBUG_RESULT(match_rule(tree, treeTypeToRule->rule));
}


void Unparser::unparse_or(const AbstractParseTree& tree, GrammarOrRules* or_rules, bool nt_with_recursive/*= false*/)
{
	DEBUG_ENTER("unparse_or");
	DEBUG_ARG_APT(tree);
	//DEBUG_ARG_G(or_rules);

	if (!nt_with_recursive && or_rules->first != 0 && or_rules->first->tree_name.empty() && or_rules->first->next == 0)
	{
		GrammarRule* rule = or_rules->first->rule;
		if (or_rules->first->single_element != 0)
		{
			unparse_rule_single(tree, rule);
		}
		else
		{
			unparse_rule(tree, rule);
		}
		return;
	}

	TreeTypeToGrammarRules* treeTypeToRules = 0;
	bool is_single = false;
	if (tree.isEmpty())
		treeTypeToRules = or_rules->emptyTreeRules;
	else if (tree.isTree())
	{
		for (TreeTypeToGrammarRules* treeNameToRules = or_rules->treeNameToRulesMap; treeNameToRules != 0; treeNameToRules = treeNameToRules->next)
		{
			if (tree.isTree(treeNameToRules->name))
			{
				treeTypeToRules = treeNameToRules;
				break;
			}
		}
	}
	else if (tree.isList())
		treeTypeToRules = or_rules->listRules;
	else
	{
		is_single = true;
		for (TreeTypeToGrammarRules* terminalToRules = or_rules->terminalToRulesMap; terminalToRules != 0; terminalToRules = terminalToRules->next)
		{
			if (_terminalUnparser->match(terminalToRules->name, tree))
			{
				treeTypeToRules = terminalToRules;
				break;
			}
		}
	}

	if (treeTypeToRules == 0)
	{
		fprintf(stderr, "Unparse error\n");
		return;
	}

	TreeTypeToGrammarRule *treeTypeToRule = 0;
	if (treeTypeToRules->nr_alternatives == 1)
		treeTypeToRule = treeTypeToRules->alternatives;
	else
	{
		/*
		fprintf(stderr, "Alternatives: %d\n", treeTypeToRules->nr_alternatives);
		if (tree.isEmpty())
			fprintf(stderr, " Tree is empty\n");
		else
			fprintf(stderr, " Tree at %d.%d\n", tree.line(), tree.column());
		*/
		for (treeTypeToRule = treeTypeToRules->alternatives; treeTypeToRule != 0; treeTypeToRule = treeTypeToRule->next)
		{
			/*
			if (treeTypeToRule->top_rule != 0)
			{
				fprintf(stderr, "  Top rule: %d.%d ", treeTypeToRule->top_rule->line, treeTypeToRule->top_rule->column);
				if (treeTypeToRule->top_rule != 0)
					treeTypeToRule->top_rule->print(stderr);
				fprintf(stderr, "\n");
			}
			if (treeTypeToRule->rule != 0)
			{
				fprintf(stderr, "  rule: %d.%d ", treeTypeToRule->rule->line, treeTypeToRule->rule->column);
				if (treeTypeToRule->rule != 0)
					treeTypeToRule->rule->print(stderr);
				fprintf(stderr, "\n");
			}
			*/

			if (match(tree, treeTypeToRule))
			{
				//fprintf(stderr, "=> matched\n");
				break;
			}
			//fprintf(stderr, "=> not matched\n");
		}
	}

	if (treeTypeToRule == 0)
	{
		fprintf(stderr, "Unparse error\n");
		return;
	}

	if (treeTypeToRule->top_rule != 0)
		unparse_rule_single(tree, treeTypeToRule->top_rule);
	else if (treeTypeToRule->rec_nt != 0)
	{
		AbstractParseTree::iterator partIt(tree);
		unparse_or(partIt, treeTypeToRule->rec_nt, treeTypeToRule->rec_nt->recursive != 0);
		partIt.next();
		unparse_rule(partIt, treeTypeToRule->rule);
	}
	else if (treeTypeToRule->is_single)
		unparse_rule_single(tree, treeTypeToRule->rule);
	else
		unparse_rule(tree, treeTypeToRule->rule);

	return;
}

void Unparser::unparse_rule_elem(const AbstractParseTree& part, GrammarRule* rule)
{
	DEBUG_ENTER("unparse_rule_elem");
	DEBUG_ARG_APT(part);
	DEBUG_ARG_G(rule);

	switch (rule->kind)
	{
		case RK_T_EOF:
			// Done
			break;
		case RK_TERM:
			_terminalUnparser->unparse(rule->text.terminal->name, part);
			break;
		case RK_WS_TERM:
			_terminalUnparser->unparseWhiteSpace(rule->text.terminal->name);
			break;
		case RK_IDENT:
			break;
		case RK_NT:
			unparse_or(part, rule->text.non_terminal, rule->text.non_terminal->recursive != 0);
			break;
		case RK_WS_NT:
			// Do not know what to do here, as parsed element has been discarded
			break;
		case RK_LIT:
			if (!rule->optional)
				_terminalUnparser->unparseLiteral(rule->str_value.val());
			break;
		case RK_OR_RULE:
			unparse_or(part, rule->text.or_rules);
			break;
	}
}

void Unparser::unparse_rule(AbstractParseTree::iterator partIt, GrammarRule* rule)
{
	DEBUG_ENTER("unparse_rule");
	//DEBUG_ARG_APT(tree);
	DEBUG_ARG_G(rule);

	for (; rule != 0; rule = rule->next)
	{
		if (rule->kind == RK_TERM || rule->kind == RK_NT || rule->kind == RK_OR_RULE)
		{
			AbstractParseTree part;
			if (partIt.more())
			{
				part = AbstractParseTree(partIt);
				partIt.next();
			}

			if (rule->optional && part.isEmpty())
				;
			else if (rule->sequential && part.isList())
			{
				bool first = true;
				for (AbstractParseTree::iterator elemIt(part); elemIt.more(); elemIt.next())
				{
					if (first)
						first = false;
					else if (!rule->chain_symbol.empty())
						_terminalUnparser->unparseLiteral(rule->chain_symbol.val());

					AbstractParseTree elem(elemIt);
					unparse_rule_elem(elem, rule);
				}
			}
			else
				unparse_rule_elem(part, rule);
		}
		else
		{
			AbstractParseTree empty_tree;
			unparse_rule_elem(empty_tree, rule);
		}
	}
}

void Unparser::unparse_rule_single(const AbstractParseTree& tree, GrammarRule* rule)
{
	DEBUG_ENTER("unparse_rule_single");
	DEBUG_ARG_APT(tree);
	DEBUG_ARG_G(rule);

	for (; rule != 0; rule = rule->next)
	{
		if (rule->kind == RK_TERM || rule->kind == RK_NT || rule->kind == RK_OR_RULE)
		{
			if (rule->optional && tree.isEmpty())
				;
			else if (rule->sequential && tree.isList())
			{
				bool first = true;
				for (AbstractParseTree::iterator elemIt(tree); elemIt.more(); elemIt.next())
				{
					if (first)
						first = false;
					else if (!rule->chain_symbol.empty())
						_terminalUnparser->unparseLiteral(rule->chain_symbol.val());

					AbstractParseTree elem(elemIt);
					unparse_rule_elem(elem, rule);
				}
			}
			else
				unparse_rule_elem(tree, rule);
		}
		else
		{
			AbstractParseTree empty_tree;
			unparse_rule_elem(empty_tree, rule);
		}
	}
}

void Unparser::unparse(const AbstractParseTree& tree, Ident root_id, AbstractStream<char> *outputStream)
{
	if (_terminalUnparser == 0)
		return;
	_terminalUnparser->init(outputStream);
	GrammarNonTerminal *root = findNonTerminal(root_id);
	unparse_or(tree, root, root->recursive != 0);
}
