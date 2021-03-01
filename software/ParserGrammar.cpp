#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Ident.h"
#include "String.h"
#include "AbstractParseTree.h"
#include "ParserGrammar.h"

GrammarCharSet::GrammarCharSet()
{
	for (int i = 0; i < 8; i++)
		_range[i] = 0;
}

void GrammarCharSet::add_range(unsigned char from, unsigned char to)
{
	for (unsigned char ch = from; ch < to; ch++)
		add_char(ch);
	add_char(to); // just in case to equals 255
}

void GrammarCharSet::RangeIterator::next()
{
	_more = false;
	for(; _ch <= 255; _ch++)
	{
		if (_char_set.contains_char(_ch))
		{
			_more = true;
			_from = _ch;
			do
			{
				_to = _ch;
				_ch++;
			}
			while (_ch <= 255 && _char_set.contains_char(_ch));
			return;
		}
	}
}

Ident TreeTypeToGrammarRules::id_empty_tree = "<empty>";
Ident TreeTypeToGrammarRules::id_list = "<list>";

GrammarRule::~GrammarRule()
{
    switch(kind)
    {
		case RK_TERM:
		case RK_WS_TERM:
			delete text.terminal;
			break;
		case RK_IDENT:
			delete text.ident;
		    break;
		case RK_CHARSET:
		case RK_AVOID:
			delete text.char_set;
			break;
		case RK_OR_RULE:
		case RK_COR_RULE:
			delete text.or_rules;
			break;
		case RK_COLOURCODING:
			delete text.colour_coding;
			break;
    }

    delete next;
}

GrammarOrRule::~GrammarOrRule()
{
	delete rule;
}

GrammarOrRules::~GrammarOrRules()
{
	while (first != 0)
	{
		GrammarOrRule *orRule = first;
		first = first->next;
		delete orRule;
	}
}

GrammarNonTerminal::~GrammarNonTerminal()
{
	while (recursive != 0)
	{
		GrammarOrRule *orRule = recursive;
		recursive = recursive->next;
		delete orRule;
	}
}

void GrammarRule::print(FILE *f)
{   
	fprintf(f, "[%ld.%ld]", line, column); 
    switch(kind)
    {   case RK_T_EOF:
            fprintf(f, "<eof> ");
            break;
        case RK_TERM:
        	fprintf(f, "<%s> ", text.terminal->name.val());
        	break;
        case RK_WS_TERM:
        	fprintf(f, "<\\%s> ", text.terminal->name.val());
        	break;
        case RK_IDENT:
            fprintf(f, "<%s> ", text.ident->terminal->name.val());
            break;
        case RK_NT:
            fprintf(f, "%s ", text.non_terminal->name.val());
            break;
        case RK_LIT:
        {
			fprintf(f, "\"");
			for (const char *s = str_value.val(); *s != '\0'; s++)
				if (*s == '\n')
					fprintf(f, "\\n");
				else
					fprintf(f, "%c", *s);
            fprintf(f, "\" ");
		}
            break;
		case RK_CHARSET:
			fprintf(f, "charset");
			break;
        case RK_OR_RULE:
            fprintf(f, "(");
            if (text.or_rules->first != 0)
            	text.or_rules->first->print(f);
            fprintf(f, ")");
            break;
        case RK_COR_RULE:
            fprintf(f, "C(");
            if (text.or_rules->first != 0)
            	text.or_rules->first->print(f);
            fprintf(f, ")");
            break;
		case RK_AVOID:
			fprintf(f, "avoidcharset");
			break;
		case RK_COLOURCODING:
			fprintf(f, "$%s", str_value.val());
			break;
    }

    if (sequential)
    {   if (chain_symbol.empty())
            fprintf(f, "SEQ ");
        else if (strcmp( chain_symbol, ",") == 0)
            fprintf(f, "LIST ");
        else
            fprintf(f, "CHAIN %s ", chain_symbol.val());
    }
    if (optional)
		fprintf(f, "OPT ");
	if (avoid)
		fprintf(f, "AVOID ");
	if (nongreedy)
		fprintf(f, "NONGREEDY ");

    if (next != 0)
    	next->print(f);
}

void GrammarOrRule::print(FILE *f, bool first)
{
	if (!first)
		fprintf(f, "|");
	if (rule != 0)
    	rule->print(f);
    if (!tree_name.empty())
       fprintf(f, "[%s]", tree_name.val());
	if (next != 0)
		next->print(f, false);
}

Ident id_nt_def = "nt_def";
Ident id_rule = "rule";
Ident id_opt = "opt";
Ident id_avoid = "avoid";
Ident id_nongreedy = "nongreedy";
Ident id_seq = "seq";
Ident id_list = "list";
Ident id_chain = "chain";
Ident id_ident = "ident";
Ident id_literal = "literal";
Ident id_local = "local";
Ident id_identalone = "identalone";
Ident id_identdef = "identdef";
Ident id_identdefadd = "identdefadd";
Ident id_identuse = "identuse";
Ident id_identfield = "identfield";
Ident id_opencontext = "opencontext";
Ident id_closecontext = "closecontext";
Ident id_wsterminal = "wsterminal";
Ident id_eof = "eof";
Ident id_charset = "charset";
Ident id_avoidlit = "avoidlit";
Ident id_colourcommand = "colourcommand";
Ident id_charrange = "charrange";
Ident id_any = "any";

GrammarRule* Grammar::make_rule(AbstractParseTree::iterator rule, GrammarOrRule* or_rule)
{
	GrammarRule* result;
 
    if (!rule.more())
        return 0;

    result = new GrammarRule;

    AbstractParseTree elem = rule;

	result->line = elem.line();
	result->column = elem.column();

    /* Is the first element optional? */
    if (elem.equalTree(id_opt))
    {   result->optional = true;
		if (elem.part(2).isTree(id_avoid))
			result->avoid = true;
		else if (elem.part(2).isTree(id_nongreedy))
			result->nongreedy = true;
        elem = elem.part(1);
    }

    /* Is it a sequential, chain or list? */
    if (elem.equalTree(id_seq))
    {   result->sequential = true;
		if (elem.part(2).isTree(id_avoid))
			result->avoid = true;
        elem = elem.part(1);
    }
    else if (elem.equalTree(id_chain))
    {   result->sequential = true;
        result->chain_symbol = elem.part(3).string();
		if (elem.part(2).isTree(id_avoid))
			result->avoid = true;
        elem = elem.part(1);
    }
    else if (elem.equalTree(id_list))
    {   result->sequential = true;
        result->chain_symbol = ",";
		if (elem.part(2).isTree(id_avoid))
			result->avoid = true;
        elem = elem.part(1);
    }

    if (elem.isIdent())
    {
       	Ident name = elem.identName();
       	
        if (name == id_eof)
            result->kind = RK_T_EOF;
        else
        {
			GrammarNonTerminal* nt = findNonTerminal(name);
        	if (nt != 0)
        	{
	            result->kind = RK_NT;
	            result->text.non_terminal = nt;
			}
			else
        	{
        		result->kind = RK_TERM;
        		result->text.terminal = new GrammarTerminal(name);
        	}
        }
    }
	else if (elem.isTree(id_wsterminal))
	{
		Ident name = elem.part(1).identName();
		GrammarNonTerminal* nt = findNonTerminal(name);
    	if (nt != 0)
    	{
            result->kind = RK_WS_NT;
            result->text.non_terminal = nt;
		}
		else
    	{
    		result->kind = RK_WS_TERM;
    		result->text.terminal = new GrammarTerminal(name);
    	}
	}
    else if (elem.isTree(id_identalone))
    {
        result->kind = RK_TERM;
        result->text.terminal = new GrammarTerminal(id_ident);
    }
    else if (elem.isTree(id_identdef))
    {
        result->kind = RK_IDENT;
        result->text.ident = new GrammarIdent(IK_IDENTDEF, elem.part(1).identName(), findTerminal(id_ident));
    }
    else if (elem.isTree(id_identdefadd))
    {
        result->kind = RK_IDENT;
        result->text.ident = new GrammarIdent(IK_IDENTDEFADD, elem.part(1).identName(), findTerminal(id_ident));
    }
    else if (elem.isTree(id_identuse))
    {
        result->kind = RK_IDENT;
        result->text.ident = new GrammarIdent(IK_IDENTUSE, elem.part(1).identName(), findTerminal(id_ident));
    }
    else if (elem.isTree(id_identfield))
    {
        result->kind = RK_IDENT;
        result->text.ident = new GrammarIdent(IK_IDENTFIELD, elem.part(1).identName(), findTerminal(id_ident));
    }
    else if (elem.isTree(id_opencontext))
    {
    	result->kind = RK_T_OPENCONTEXT;
    }
    else if (elem.isTree(id_closecontext))
    {
		result->kind = RK_T_CLOSECONTEXT;
	}
    else if (elem.isTree(id_literal))
    {
        result->kind = RK_LIT;
        result->str_value = elem.part(1).string();
		if (!elem.part(2).isTree(id_local))
			addLiteral(elem.part(1).stringValue());
    }
	else if (elem.isTree(id_charset))
	{
		GrammarCharSet* char_set = new GrammarCharSet;
		make_char_set(elem.part(1), char_set);
		result->kind = RK_CHARSET;
		result->text.char_set = char_set;
	}
	else if (elem.isTree(id_avoid))
	{
		GrammarCharSet* char_set = new GrammarCharSet;
		make_char_set(elem.part(1), char_set);
		result->kind = RK_AVOID;
		result->text.char_set = char_set;
	}
	else if (elem.isTree(id_avoidlit))
	{
        result->kind = RK_AVOIDLIT;
        result->str_value = elem.part(1).string();
	}
	else if (elem.isTree(id_colourcommand))
	{
		result->kind = RK_COLOURCODING;
		result->text.colour_coding = new GrammarColourCoding(elem.part(1).charValue(), elem.part(2).intValue(), elem.part(3).intValue());
	}
    else if (elem.isList())
    {
        result->kind = RK_OR_RULE;
        result->text.or_rules = new GrammarOrRules;
		result->text.or_rules->first = make_or_rule(AbstractParseTree::iterator(elem));
	}
	else
		printf("Error\n");
	
	if (result->kind == RK_TERM || result->kind == RK_NT || result->kind == RK_OR_RULE)
	{
		or_rule->single_element = ++or_rule->nr_active == 1 ? result : 0;
	}

    rule.next();
    result->next = make_rule(rule, or_rule);

    return result;
}

void
Grammar::make_char_set(AbstractParseTree char_set_rule, GrammarCharSet *char_set)
{
	for (AbstractParseTree::iterator it(char_set_rule.part(1)); it.more(); it.next())
	{
		AbstractParseTree part(it);
		if (part.isChar())
			char_set->add_char(part.charValue());
		else if (part.isTree(id_charrange))
			char_set->add_range(part.part(1).charValue(), part.part(2).charValue());
		else if (part.isTree(id_any))
			char_set->add_range(0, 255);
	}
	for (AbstractParseTree::iterator it(char_set_rule.part(2)); it.more(); it.next())
	{
		AbstractParseTree part(it);
		char_set->remove_char(part.charValue());
	}
}

GrammarRule* Grammar::make_rule(CombinedRule* rules)
{
	GrammarRule* result;

	bool all_equivalent = rules->rule.more();
	for (CombinedRule* rule = rules->next; rule != 0 && all_equivalent; rule = rule->next)
		if (!rule->rule.more() || !equivalent(rules->rule, rule->rule))
			all_equivalent = false;

    result = new GrammarRule;
	if (all_equivalent)
	{
		AbstractParseTree elem = rules->rule;
		if (elem.isIdent())
		{
       		Ident name = elem.identName();
	       	
			if (name == id_eof)
				result->kind = RK_T_EOF;
			else
			{
				GrammarNonTerminal* nt = findNonTerminal(name);
        		if (nt != 0)
        		{
					result->kind = RK_NT;
					result->text.non_terminal = nt;
				}
				else
        		{
        			result->kind = RK_TERM;
        			result->text.terminal = new GrammarTerminal(name);
        		}
			}
		}
		else if (elem.isTree(id_wsterminal))
		{
			result->kind = RK_WS_TERM;
			result->text.terminal = new GrammarTerminal(elem.part(1).identName());
		}
		else if (elem.isTree(id_identalone))
		{
			result->kind = RK_TERM;
			result->text.terminal = new GrammarTerminal(id_ident);
		}

		for (CombinedRule* rule = rules; rule != 0; rule = rule->next)
			rule->rule.next();

		result->next = make_rule(rules);
	}
	else
	{
		// One deep implementation
		result->kind = RK_COR_RULE;
		result->text.or_rules = new GrammarOrRules;
		GrammarOrRule** ref_last = &result->text.or_rules->first;
		for (CombinedRule* rule = rules; rule != 0; rule = rule->next)
		{
			GrammarOrRule *new_or_rule = new GrammarOrRule;
			new_or_rule->tree_name = rule->tree_name;
			new_or_rule->rule = make_rule(rule->rule, new_or_rule);
			(*ref_last) = new_or_rule;
			ref_last = &(*ref_last)->next;
		}
	}

	return result;
}

bool Grammar::equivalent(const AbstractParseTree& lhs, const AbstractParseTree& rhs)
{
	if (lhs.isIdent())
		return rhs.isIdent() && lhs.identName() == rhs.identName();
	if (rhs.isIdent())
		return false;
	if (lhs.isTree(id_wsterminal))
		return rhs.isTree(id_wsterminal) && lhs.part(1).identName() == rhs.part(1).identName();
	if (rhs.isTree(id_wsterminal))
		return false;
	if (lhs.isTree(id_identalone))
		return rhs.isTree(id_identalone);

	return false;
}

GrammarOrRule* Grammar::make_or_rule(AbstractParseTree::iterator or_rule)
{
    if (!or_rule.more())
        return 0;

    {   AbstractParseTree rule = or_rule;
        if (rule.isTree(id_rule))
        {   GrammarOrRule* result = new GrammarOrRule;

            if (rule.nrParts() == 0)
            {
                result->rule = 0;
            }
            else
            {   result->rule = make_rule(AbstractParseTree::iterator(rule.part(1)), result);
				if (rule.part(2).isIdent())
					result->tree_name = rule.part(2).identName();
            }
            or_rule.next();
            result->next = make_or_rule(or_rule);
            return result;
        }
    }
 
 	or_rule.next();
    return make_or_rule(or_rule);
}

void Grammar::loadGrammar(const AbstractParseTree& root)
{
	_all_nt = 0;
	GrammarNonTerminal **ref_nt = &_all_nt;

    for (AbstractParseTree::iterator rules1(root); rules1.more(); rules1.next())
    {
    	AbstractParseTree rule(rules1);
        if (rule.isTree(id_nt_def))
        {   Ident nt_name = rule.part(1).identName();
			
			if (findNonTerminal(nt_name) == 0)
			{
				*ref_nt = new GrammarNonTerminal(nt_name);
				ref_nt = &(*ref_nt)->next;
			}
		}
	}

    for (AbstractParseTree::iterator rules(root); rules.more(); rules.next())
    {
    	AbstractParseTree rule(rules);
        if (rule.isTree(id_nt_def))
        {   Ident nt_name = rule.part(1).identName();
            GrammarNonTerminal* nt = findNonTerminal(nt_name);
            GrammarOrRule* *p_first = &nt->first;
            GrammarOrRule* *p_recursive = &nt->recursive;

            while (*p_first != 0)
                p_first = &(*p_first)->next;
            while (*p_recursive != 0)
                p_recursive = &(*p_recursive)->next;

            for (AbstractParseTree::iterator or_rule(rule.part(2)); or_rule.more();)
            {   AbstractParseTree rule(or_rule);
                if (rule.isTree(id_rule))
                {   GrammarOrRule* new_or_rule = new GrammarOrRule;

                    if (rule.nrParts() == 0)
                    {   new_or_rule->rule = 0;
                        new_or_rule->next = 0;
                        *p_first = new_or_rule;
                        p_first = &new_or_rule->next;
						or_rule.next();
                    }
                    else
                    {   AbstractParseTree::iterator parts(rule.part(1));
						Ident tree_name;
						if (rule.part(2).isIdent())
							tree_name = rule.part(2).stringValue();
                        new_or_rule->next = 0;
                        if (parts.more() && AbstractParseTree(parts).isIdent(nt_name))
                        {
                        	parts.next();
							new_or_rule->tree_name = tree_name;
							new_or_rule->nr_active = 1;
                            new_or_rule->rule = make_rule(parts, new_or_rule);
                            *p_recursive = new_or_rule;
                            p_recursive = &new_or_rule->next;
							or_rule.next();
                        }
                        else
                        {
							int nr_combined_rules = 1;
							CombinedRule* combinedRules = 0;
							if (_for_unparse || !parts.more())
								or_rule.next();
							else
							{
								combinedRules = new CombinedRule(parts, tree_name);
								CombinedRule** ref_next = &combinedRules->next;
								for (;;)
								{
									or_rule.next();
									if (!or_rule.more())
										break;
									AbstractParseTree next_rule(or_rule);
									if (   next_rule.isTree(id_rule)
										&& next_rule.part(1).nrParts() > 1
										&& equivalent(parts, next_rule.part(1).part(0)))
									{
										nr_combined_rules++;
										Ident next_tree_name;
										if (next_rule.part(2).isIdent())
											next_tree_name = next_rule.part(2).stringValue();

										(*ref_next) = new CombinedRule(next_rule.part(1), next_tree_name);
										ref_next = &(*ref_next)->next;
									}
									else
										break;
								}
							}
							if (nr_combined_rules == 1)
							{
								new_or_rule->tree_name = tree_name;
								new_or_rule->rule = make_rule(parts, new_or_rule);
							}
							else
								new_or_rule->rule = make_rule(combinedRules);
							*p_first = new_or_rule;
							p_first = &new_or_rule->next;
							delete combinedRules;
                        }
                    }                     
                }
            }
        }
    }
	if (false)
    {	GrammarNonTerminal* nt;
    
    	for (nt = _all_nt; nt != 0; nt = nt->next)
    		if (nt->first == 0 && nt->recursive == 0)
    		    printf("Non-terminal '%s' has no rule.\n", nt->name.val());
    		else
    		{
    			printf("Non-terminal '%s'\n", nt->name.val());
    			
    			for (GrammarOrRule* o_r = nt->first; o_r != 0; o_r = o_r->next)
    			{
    				printf(" First: ");
    				if (o_r->rule != 0)
    					o_r->rule->print(stdout);
    				printf("\n");
    			}
    			for (GrammarOrRule* o_r2 = nt->recursive; o_r2 != 0; o_r2 = o_r2->next)
    			{
    				printf(" Recursive: ");
    				if (o_r2->rule != 0)
    					o_r2->rule->print(stdout);
    				printf("\n");
    			}
    		}
    }		    
}

GrammarNonTerminal* Grammar::findNonTerminal(Ident name)
{
	for (GrammarNonTerminal* nt = _all_nt; nt != 0; nt = nt->next)
		if (nt->name == name)
			return nt;

	return 0;
}

GrammarNonTerminal* Grammar::addNonTerminal(Ident name)
{
	GrammarNonTerminal** ref_nt = &_all_nt;
	for (; (*ref_nt) != 0; ref_nt = &(*ref_nt)->next)
		if ((*ref_nt)->name == name)
			return (*ref_nt);

	*ref_nt = new GrammarNonTerminal(name);
	return *ref_nt;
}

GrammarTerminal* Grammar::findTerminal(Ident name)
{
	GrammarTerminal** ref_t = &_all_t;
	for (; (*ref_t) != 0; ref_t = &(*ref_t)->next)
		if ((*ref_t)->name == name)
			return (*ref_t);
	
	*ref_t = new GrammarTerminal(name);

	return *ref_t;
}

class GrammarLiteral
{
public:
	GrammarLiteral(Ident n_value) : value(n_value), next(0) {}
	Ident value;
	GrammarLiteral* next;
};

Grammar::~Grammar()
{
	while (_all_nt != 0)
	{
		GrammarNonTerminal* nonTerm = _all_nt;
		_all_nt = _all_nt->next;
		delete nonTerm;
	}
	while (_all_t != 0)
	{
		GrammarTerminal* term = _all_t;
		_all_t = _all_t->next;
		delete term;
	}
	while (_all_l != 0)
	{
		GrammarLiteral* gramLit = _all_l;
		_all_l = _all_l->next;
		delete gramLit;
	}
}

void Grammar::addLiteral(Ident literal)
{
	GrammarLiteral** ref_l = &_all_l;
	for (; (*ref_l) != 0; ref_l = &(*ref_l)->next)
		if ((*ref_l)->value == literal)
			return;
	
	*ref_l = new GrammarLiteral(literal);
}

bool Grammar::isLiteral(Ident literal)
{
	for (GrammarLiteral* l = _all_l; l != 0; l = l->next)
		if (l->value == literal)
			return true;

	return false;
}

class SearchedNonTerminals
{
public:
	SearchedNonTerminals(GrammarNonTerminal *nonTerminal, SearchedNonTerminals *prev)
		: _nonTerminal(nonTerminal), _prev(prev) {}
	bool contains(GrammarNonTerminal *nonTerminal)
	{
		if (_nonTerminal == nonTerminal)
			return true;
		if (_prev == 0)
			return false;
		return _prev->contains(nonTerminal);
	}
private:
	GrammarNonTerminal *_nonTerminal;
	SearchedNonTerminals *_prev;
};

void addToTreeTypeToRulesMap(Ident tree_type, TreeTypeToGrammarRules** ref_treeTypeToRulesMap, GrammarNonTerminal *rec_nt, GrammarOrRule* or_rule, GrammarRule *top_rule, AbstractUnparseErrorCollector *unparseErrorCollector)
{
	GrammarRule *rule = or_rule->rule;
	for (; *ref_treeTypeToRulesMap != 0; ref_treeTypeToRulesMap = &(*ref_treeTypeToRulesMap)->next)
		if ((*ref_treeTypeToRulesMap)->name == tree_type)
		{
			for (TreeTypeToGrammarRule* alternative = (*ref_treeTypeToRulesMap)->alternatives; alternative != 0; alternative = alternative->next)
				if (alternative->rule == rule)
				{
					if (alternative->top_rule != top_rule)
						unparseErrorCollector->warningTypeReachedThroughDifferentPaths(tree_type, or_rule->rule, alternative->top_rule, top_rule);
					return;
				}
			unparseErrorCollector->errorDifferentRulesWithSameType(tree_type, (*ref_treeTypeToRulesMap)->alternatives->rule, rule);
			(*ref_treeTypeToRulesMap)->add(new TreeTypeToGrammarRule(top_rule, rec_nt, or_rule));
			return;
		}
	*ref_treeTypeToRulesMap = new TreeTypeToGrammarRules(tree_type);
	(*ref_treeTypeToRulesMap)->add(new TreeTypeToGrammarRule(top_rule, rec_nt, or_rule));
}

bool noNonLiterals(GrammarRule* rule)
{
	for (; rule != 0; rule = rule->next)
		if (rule->kind != RK_LIT)
			return false;
	return true;
}

void processOrRules(GrammarOrRules *or_rules, GrammarRule *top_rule, GrammarNonTerminal *rec_nt, GrammarOrRule* or_rule, AbstractUnparseErrorCollector *unparseErrorCollector)
{
	if (!or_rule->tree_name.empty())
		addToTreeTypeToRulesMap(or_rule->tree_name, &or_rules->treeNameToRulesMap, rec_nt, or_rule, top_rule, unparseErrorCollector);
	else
	{
		GrammarRule* single_rule = or_rule->single_element; 
		if (rec_nt == 0 && (noNonLiterals(or_rule->rule) || (single_rule != 0 && single_rule->optional)))
			addToTreeTypeToRulesMap(TreeTypeToGrammarRules::id_empty_tree, &or_rules->emptyTreeRules, rec_nt, or_rule, top_rule, unparseErrorCollector);
		if (rec_nt == 0 && single_rule != 0 && single_rule->kind == RK_TERM && !single_rule->sequential)
			addToTreeTypeToRulesMap(single_rule->text.terminal->name, &or_rules->terminalToRulesMap, rec_nt, or_rule, top_rule, unparseErrorCollector);
		if (rec_nt != 0 || or_rule->nr_active > 1 || (single_rule != 0 && single_rule->sequential))
			addToTreeTypeToRulesMap(TreeTypeToGrammarRules::id_list, &or_rules->listRules, rec_nt, or_rule, top_rule, unparseErrorCollector);
	}
}

void processNonTerminal(GrammarNonTerminal *nt, GrammarRule *top_rule, GrammarOrRules *or_rules, SearchedNonTerminals &searched_nt, AbstractUnparseErrorCollector *unparseErrorCollector)
{
	for (GrammarOrRule* or_rule = nt->first; or_rule != 0; or_rule = or_rule->next)
		processOrRules(or_rules, top_rule, 0, or_rule, unparseErrorCollector);

	for (GrammarOrRule* or_rule = nt->recursive; or_rule != 0; or_rule = or_rule->next)
		processOrRules(or_rules, top_rule, nt, or_rule, unparseErrorCollector);

	for (GrammarOrRule* or_rule = nt->first; or_rule != 0; or_rule = or_rule->next)
		if (or_rule->tree_name.empty())
		{
			GrammarRule* single_rule = or_rule->single_element; 
			if (single_rule != 0 && single_rule->kind == RK_NT  && !single_rule->sequential)
			{
				GrammarNonTerminal *child_nt = single_rule->text.non_terminal;
				if (!searched_nt.contains(child_nt))
				{
					GrammarRule *child_top_rule = top_rule != 0 ? top_rule : or_rule->rule->next != 0 ? or_rule->rule : 0;
					SearchedNonTerminals child_searched_nt(nt, &searched_nt);
					processNonTerminal(child_nt, child_top_rule, or_rules, child_searched_nt, unparseErrorCollector);
				}
			}
		}
}

void searchForOrRules(GrammarRule *rule, AbstractUnparseErrorCollector *unparseErrorCollector)
{
	for (; rule != 0; rule = rule->next)
		if (rule->kind == RK_OR_RULE)
		{
			GrammarOrRules *or_rules = rule->text.or_rules;
			
			for (GrammarOrRule* or_rule = or_rules->first; or_rule != 0; or_rule = or_rule->next)
				processOrRules(or_rules, 0, 0, or_rule, unparseErrorCollector);

			for (GrammarOrRule* or_rule = or_rules->first; or_rule != 0; or_rule = or_rule->next)
				if (or_rule->tree_name.empty())
				{
					GrammarRule* single_rule = or_rule->single_element; 
					if (single_rule != 0 && single_rule->kind == RK_NT  && !single_rule->sequential)
					{
						GrammarNonTerminal *child_nt = single_rule->text.non_terminal;
						GrammarRule *child_top_rule = or_rule->rule->next != 0 ? or_rule->rule : 0;
						SearchedNonTerminals child_searched_nt(0, 0);
						processNonTerminal(child_nt, child_top_rule, or_rules, child_searched_nt, unparseErrorCollector);
					}
				}
			for (GrammarOrRule* or_rule = or_rules->first; or_rule != 0; or_rule = or_rule->next)
				searchForOrRules(or_rule->rule, unparseErrorCollector);
		}
}

void Grammar::loadGrammarForUnparse(const AbstractParseTree& root, AbstractUnparseErrorCollector *unparseErrorCollector)
{
	_for_unparse = true;
	loadGrammar(root);

	for (GrammarNonTerminal* nt = _all_nt; nt != 0; nt = nt->next)
	{
		SearchedNonTerminals searched_nt(nt, 0);
		processNonTerminal(nt, 0, nt, searched_nt, unparseErrorCollector);

		for (GrammarOrRule* or_rule = nt->first; or_rule != 0; or_rule = or_rule->next)
			searchForOrRules(or_rule->rule, unparseErrorCollector);

		for (GrammarOrRule* or_rule = nt->recursive; or_rule != 0; or_rule = or_rule->next)
			searchForOrRules(or_rule->rule, unparseErrorCollector);
	}
}

class CodeGenerator
{
public:
	CodeGenerator(FILE* f) : _f(f), _indent(0) {}
	virtual void prologue(const char* class_name) {}
	virtual void epilogue() {}
	virtual void nt_def(const char* name) {}
	virtual void nt(const char* name) {}
	virtual void lit(const char* value, bool local) {}
	virtual void charset() {}
	virtual void add_char(unsigned char ch) {}
	virtual void add_range(unsigned char from, unsigned char to) {}
	virtual void term(const char* name) {}
	virtual void wsterm(const char* name) {}
	virtual void wsnt(const char* name) {}
	virtual void chain(const char* sym) {}
	virtual void seq() {}
	virtual void opt() {}
	virtual void avoid() {}
	virtual void nongreedy() {}
	virtual void open(bool combined) {}
	virtual void close() {}
	virtual void or() {}
	virtual void rec_or() {}
	virtual void eof() {}
	virtual void tree(const char* name) {}
	virtual void notSupported(const char* kind) {}
protected:
	void startLine()
	{
		fprintf(_f, "\n");
		for (int i = 0; i < _indent; i++)
			fprintf(_f, "\t");
	}
	void printString(const char* str)
	{
		for (const char *s = str; *s != '\0'; s++)
			if (*s == '\n')
				fprintf(_f, "\\n");
			else if (*s == '\t')
				fprintf(_f, "\\t");
			else if (*s == '\\')
				fprintf(_f, "\\\\");
			else
				fprintf(_f, "%c", *s);
	}
	FILE* _f;
	int _indent;
};

class CodeGeneratorCalls : public CodeGenerator
{
public:
	CodeGeneratorCalls(FILE* f) : CodeGenerator(f) {}
	virtual void prologue(const char* class_name) override
	{
		fprintf(_f, "#include <stdio.h>\n");
		fprintf(_f, "#include <string.h>\n");
		fprintf(_f, "#include \"Ident.h\"\n");
		fprintf(_f, "#include \"String.h\"\n");
		fprintf(_f, "#include \"AbstractParseTree.h\"\n");
		fprintf(_f, "#include \"ParserGrammar.h\"\n\n");

		fprintf(_f, "class %s : public GrammarLoader\n", class_name);
		fprintf(_f, "{\n");
		fprintf(_f, "public:\n");
		fprintf(_f, "\t%s(Grammar* grammar) : GrammarLoader(grammar) \n\t{", class_name);
		_indent = 2;
	}
	virtual void epilogue() override
	{
		fprintf(_f, "\n\t}\n};\n");
	}
	virtual void nt_def(const char* name) override
	{
		fprintf(_f, "\n");
		startLine();
		fprintf(_f, "nt_def(\"");
		printString(name);
		fprintf(_f, "\");");
	}
	virtual void nt(const char* name) override { _call_to("nt", name); }
	virtual void lit(const char* value, bool local) override
	{
		fprintf(_f, " lit(\"");
		printString(value);
		fprintf(_f, "\"%s);", local ? "/*local*/true" : "");
	}
	virtual void charset() override { fprintf(_f, " charset();"); }
	virtual void add_char(unsigned char ch) override { fprintf(_f, " add_char(%d);", ch); }
	virtual void add_range(unsigned char from, unsigned char to) override { fprintf(_f, " add_range(%d, %d);", from, to); }
	virtual void term(const char* name) override { _call_to("term", name); }
	virtual void wsterm(const char* name) override { _call_to("ws", name); }
	virtual void wsnt(const char* name) override { _call_to("ws_nt", name); }
	virtual void chain(const char* sym) override { _call_to("chain", sym); }
	virtual void seq() override { fprintf(_f, " seq();"); }
	virtual void opt() override { fprintf(_f, " opt();"); }
	virtual void avoid() override { fprintf(_f, " avoid();"); }
	virtual void nongreedy() override { fprintf(_f, " nongreedy();"); }
	virtual void open(bool combined) override
	{
		startLine();
		fprintf(_f, "open(%s);", combined ? "/*combined*/true" : "");
		_indent++;
	}
	virtual void close() override { _indent--; startLine(); fprintf(_f, "close();");}
	virtual void or() override { startLine(); fprintf(_f, "or();"); }
	virtual void rec_or() override { startLine(); fprintf(_f, "rec_or();"); }
	virtual void eof() override { fprintf(_f, " eof();"); }
	virtual void tree(const char* name) override { _call_to("tree", name); }

	virtual void notSupported(const char* kind) override
	{
		fprintf(_f, " /* %s not supported*/", kind);
	}
private:
	void _call_to(const char* fn, const char* value)
	{
		fprintf(_f, " %s(\"", fn);
		printString(value);
		fprintf(_f, "\");");
	}
};

class CodeGeneratorDefines : public CodeGenerator
{
public:
	CodeGeneratorDefines(FILE* f) : CodeGenerator(f) {}
	virtual void prologue(const char* class_name) override
	{
		fprintf(_f, "#include <stdio.h>\n");
		fprintf(_f, "#include <string.h>\n");
		fprintf(_f, "#include \"Ident.h\"\n");
		fprintf(_f, "#include \"String.h\"\n");
		fprintf(_f, "#include \"AbstractParseTree.h\"\n");
		fprintf(_f, "#include \"ParserGrammar.h\"\n\n");

		fprintf(_f, "#define NT_DEF(name) nt = grammar.addNonTerminal(Ident(name)); ref_or_rule = &nt->first; ref_rec_or_rule = &nt->recursive;\n");
		fprintf(_f, "#define NEW_GR(K) rule = *ref_rule = new GrammarRule(); ref_rule = &rule->next; rule->kind = K;\n");
		fprintf(_f, "#define NT(name) NEW_GR(RK_NT) rule->text.non_terminal = grammar.addNonTerminal(Ident(name));\n");
		fprintf(_f, "#define TERM(name) NEW_GR(RK_TERM) rule->text.terminal = new GrammarTerminal(Ident(name));\n");
		fprintf(_f, "#define LLIT(sym) NEW_GR(RK_LIT) rule->str_value = sym;\n");
		fprintf(_f, "#define LIT(sym) LLIT(sym) grammar.addLiteral(Ident(sym));\n");
		fprintf(_f, "#define CHARSET(sym) NEW_GR(RK_CHARSET) rule->text.char_set = new GrammarCharSet();\n");
		fprintf(_f, "#define ADD_CHAR(C) rule->text.char_set->add_char(C);\n");
		fprintf(_f, "#define ADD_RANGE(F,T) rule->text.char_set->add_range(F, T);\n");
		fprintf(_f, "#define WS(name) NEW_GR(RK_WS_TERM) rule->text.terminal = new GrammarTerminal(Ident(name));\n");
		fprintf(_f, "#define WS_NT(name) NEW_GR(RK_WS_NT) rule->text.non_terminal = grammar.addNonTerminal(Ident(name));\n");
		fprintf(_f, "#define G_EOF NEW_GR(RK_T_EOF)\n");
		fprintf(_f, "#define SEQ rule->sequential = true;\n");
		fprintf(_f, "#define CHAIN(sym) SEQ rule->chain_symbol = sym;\n");
		fprintf(_f, "#define OPT rule->optional = true;\n");
		fprintf(_f, "#define AVOID rule->avoid = true;\n");
		fprintf(_f, "#define NONGREEDY rule->nongreedy = true;\n");
		fprintf(_f, "#define OPEN NEW_GR(RK_OR_RULE) rule->text.or_rules = new GrammarOrRules; { GrammarOrRule** ref_or_rule = &rule->text.or_rules->first; GrammarOrRule* or_rule; GrammarRule** ref_rule; GrammarRule* rule;\n");
		fprintf(_f, "#define OPEN_C NEW_GR(RK_COR_RULE) rule->text.or_rules = new GrammarOrRules; { GrammarOrRule** ref_or_rule = &rule->text.or_rules->first; GrammarOrRule* or_rule; GrammarRule** ref_rule; GrammarRule* rule;\n");
		fprintf(_f, "#define OR or_rule = *ref_or_rule = new GrammarOrRule; ref_or_rule = &or_rule->next; ref_rule = &or_rule->rule;\n");
		fprintf(_f, "#define CLOSE }\n");
		fprintf(_f, "#define REC_OR or_rule = *ref_rec_or_rule = new GrammarOrRule; ref_rec_or_rule = &or_rule->next; ref_rule = &or_rule->rule;\n");
		fprintf(_f, "#define TREE(name) or_rule->tree_name = Ident(name);\n");
		fprintf(_f, "\n");

		fprintf(_f, "void %s(Grammar& grammar)\n{", class_name);
		fprintf(_f, "\n\tGrammarNonTerminal* nt;");
		fprintf(_f, "\n\tGrammarOrRule** ref_or_rule;");
		fprintf(_f, "\n\tGrammarOrRule** ref_rec_or_rule;");
		fprintf(_f, "\n\tGrammarOrRule* or_rule;");
		fprintf(_f, "\n\tGrammarRule** ref_rule;");
		fprintf(_f, "\n\tGrammarRule* rule;");

		_indent = 1;
	}
	virtual void epilogue() override
	{
		fprintf(_f, "\n}\n");
	}
	virtual void nt_def(const char* name) override
	{
		fprintf(_f, "\n");
		startLine();
		fprintf(_f, "NT_DEF(\"");
		printString(name);
		fprintf(_f, "\")");
	}
	virtual void nt(const char* name) override { _call_to("NT", name); }
	virtual void lit(const char* value, bool local) override { _call_to(local ? "LLIT" : "LIT", value); }
	virtual void charset() override { fprintf(_f, " CHARSET"); }
	virtual void add_char(unsigned char ch) override { fprintf(_f, " ADD_CHAR(%d)", ch); }
	virtual void add_range(unsigned char from, unsigned char to) override { fprintf(_f, " ADD_RANGE(%d, %d)", from, to); }
	virtual void term(const char* name) override { _call_to("TERM", name); }
	virtual void wsterm(const char* name) override { _call_to("WS", name); }
	virtual void wsnt(const char* name) override { _call_to("WS_NT", name); }
	virtual void chain(const char* sym) override { _call_to("CHAIN", sym); }
	virtual void seq() override { fprintf(_f, " SEQ"); }
	virtual void opt() override { fprintf(_f, " OPT"); }
	virtual void avoid() override { fprintf(_f, " AVOID"); }
	virtual void nongreedy() override { fprintf(_f, " NONGREEDY"); }
	virtual void open(bool combined) override
	{
		startLine();
		fprintf(_f, "OPEN%s", combined ? "_C" : "");
		_indent++;
	}
	virtual void close() override { _indent--; startLine(); fprintf(_f, "CLOSE");}
	virtual void or() override { startLine(); fprintf(_f, "OR"); }
	virtual void rec_or() override { startLine(); fprintf(_f, "REC_OR"); }
	virtual void eof() override { fprintf(_f, " G_EOF"); }
	virtual void tree(const char* name) override { _call_to("TREE", name); }

	virtual void notSupported(const char* kind) override
	{
		fprintf(_f, " /* %s not supported*/", kind);
	}
private:
	void _call_to(const char* fn, const char* value)
	{
		fprintf(_f, " %s(\"", fn);
		printString(value);
		fprintf(_f, "\")");
	}
};


void Grammar::outputCodeFor(GrammarOrRule* or_rule, CodeGenerator &codeGenerator)
{
	if (or_rule->rule != 0)
	{
		for (GrammarRule* rule = or_rule->rule; rule != 0; rule = rule->next)
		{
			switch (rule->kind)
			{
			case RK_NT:
				codeGenerator.nt(rule->text.non_terminal->name.val());
				break;
			case RK_LIT:
				codeGenerator.lit(rule->str_value, !isLiteral(Ident(rule->str_value)));
				break;
			case RK_CHARSET:
				{
					codeGenerator.charset();
					for (GrammarCharSet::RangeIterator it(*rule->text.char_set); it.more(); it.next())
						if (it.from() < it.to())
							codeGenerator.add_range(it.from(), it.to());
						else
							codeGenerator.add_char(it.from());
				}
				break;
			case RK_AVOID:
				codeGenerator.notSupported("RK_AVOID");
				break;
			case RK_AVOIDLIT:
				codeGenerator.notSupported("RK_AVOIDLIT");
				break;
			case RK_OR_RULE:
			case RK_COR_RULE:
				codeGenerator.open(rule->kind == RK_COR_RULE);
				for (GrammarOrRule* or_rule = rule->text.or_rules->first; or_rule != 0; or_rule = or_rule->next)
				{
					codeGenerator.or();
					outputCodeFor(or_rule, codeGenerator);
				}
				codeGenerator.close();
				break;
			case RK_T_EOF:
				codeGenerator.eof();
				break;
			case RK_TERM:
				codeGenerator.term(rule->text.terminal->name.val());
				break;
			case RK_WS_TERM:
				codeGenerator.wsterm(rule->text.terminal->name.val());
				break;
			case RK_WS_NT:
				codeGenerator.wsnt(rule->text.terminal->name.val());
				break;
			case RK_IDENT:
				codeGenerator.notSupported("RK_IDENT");
				break;
			case RK_T_OPENCONTEXT:
				codeGenerator.notSupported("RK_T_OPENCONTEXT");
				break;
			case RK_T_CLOSECONTEXT:
				codeGenerator.notSupported("RK_T_CLOSECONTEXT");
				break;
			case RK_COLOURCODING:
				codeGenerator.notSupported("RK_COLOURCODING");
				break;
			default:
				break;
			}
			if (!rule->chain_symbol.empty())
				codeGenerator.chain(rule->chain_symbol.val());
			else if (rule->sequential)
				codeGenerator.seq();
			if (rule->optional)
				codeGenerator.opt();
			if (rule->avoid)
				codeGenerator.avoid();
			if (rule->nongreedy)
				codeGenerator.nongreedy();
		}
	}
	if (!or_rule->tree_name.empty())
		codeGenerator.tree(or_rule->tree_name.val());
}

void Grammar::outputGrammarAsCode(FILE* fout, const char* name)
{
	/*
	for (GrammarLiteral* lit = _all_l; lit != 0; lit = lit->next)
	{
		startLine(fout);
		fprintf(fout, "addLiteral(Ident(\""); 
		printString(fout, lit->value.val());
		fprintf(fout, "\"));");
	}
	fprintf(fout, "\n");
	for (GrammarTerminal* term = _all_t; term != 0; term = term->next)
	{
		startLine(fout);
		fprintf(fout, "*findTerminal(Ident(\""); 
		printString(fout, term->name.val());
		fprintf(fout, "\"));");
	}
	fprintf(fout, "\n");
	*/
	CodeGeneratorCalls codeGenerator(fout);
	codeGenerator.prologue(name);
	for (GrammarNonTerminal* non_term = _all_nt; non_term != 0; non_term = non_term->next)
	{
		codeGenerator.nt_def(non_term->name.val());
		for (GrammarOrRule* or_rule = non_term->first; or_rule != 0; or_rule = or_rule->next)
		{
			codeGenerator.or();
			outputCodeFor(or_rule, codeGenerator);
		}
		for (GrammarOrRule* or_rule = non_term->recursive; or_rule != 0; or_rule = or_rule->next)
		{
			codeGenerator.rec_or();
			outputCodeFor(or_rule, codeGenerator);
		}
	}
	codeGenerator.epilogue();
}

void GrammarLoader::nt_def(const char* name)
{
	_nt = _grammar->addNonTerminal(Ident(name));
	_c = _contexts;
	_c->ref_or_rule = &_nt->first;
	_ref_rec_or_rule = &_nt->recursive;
	_c->or_rule = 0;
	_c->ref_rule = 0;
	_c->rule = 0;
}

void GrammarLoader::_new_elem()
{
	_c->rule = *_c->ref_rule = new GrammarRule();
	_c->ref_rule = &_c->rule->next;
}

void GrammarLoader::nt(const char* name)
{
	_new_elem();
	_c->rule->kind = RK_NT;
	_c->rule->text.non_terminal = _grammar->addNonTerminal(Ident(name));
}

void GrammarLoader::term(const char* name)
{
	_new_elem();
	_c->rule->kind = RK_TERM;
	_c->rule->text.terminal = new GrammarTerminal(Ident(name));
}

void GrammarLoader::lit(const char* sym, bool local)
{
	_new_elem();
	_c->rule->kind = RK_LIT;
	_c->rule->str_value = sym;
	if (!local)
		_grammar->addLiteral(Ident(sym));
}

void GrammarLoader::ws_nt(const char* name)
{
	_new_elem();
	_c->rule->kind = RK_WS_NT;
	_c->rule->text.non_terminal = _grammar->addNonTerminal(Ident(name));
}

void GrammarLoader::ws(const char* name)
{
	_new_elem();
	_c->rule->kind = RK_WS_TERM;
	_c->rule->text.terminal = new GrammarTerminal(Ident(name));
}

void GrammarLoader::charset()
{
	_new_elem();
	_c->rule->kind = RK_CHARSET;
	_c->rule->text.char_set = new GrammarCharSet;
}

void GrammarLoader::add_char(unsigned char ch)
{
	if (_c->rule->kind == RK_CHARSET)
		_c->rule->text.char_set->add_char(ch);
}

void GrammarLoader::add_range(unsigned char from, unsigned char to)
{
	if (_c->rule->kind == RK_CHARSET)
		_c->rule->text.char_set->add_range(from, to);
}

void GrammarLoader::add_any()
{
	if (_c->rule->kind == RK_CHARSET)
		_c->rule->text.char_set->add_range(0, 255);
}

void GrammarLoader::remove_char(unsigned char ch)
{
	if (_c->rule->kind == RK_CHARSET)
		_c->rule->text.char_set->remove_char(ch);
}

void GrammarLoader::eof()
{
	_new_elem();
	_c->rule->kind = RK_T_EOF;
}

void GrammarLoader::chain(const char* sym)
{
	_c->rule->sequential = true;
	_c->rule->chain_symbol = sym;
}

void GrammarLoader::seq()
{
	_c->rule->sequential = true;
}

void GrammarLoader::opt()
{
	_c->rule->optional = true;
}

void GrammarLoader::avoid()
{
	_c->rule->avoid = true;
}

void GrammarLoader::nongreedy()
{
	_c->rule->nongreedy = true;
}

void GrammarLoader::open(bool combined)
{
	_new_elem();
	_c->rule->kind = combined ? RK_COR_RULE : RK_OR_RULE;
	_c->rule->text.or_rules = new GrammarOrRules;
	_c[1].ref_or_rule = &_c->rule->text.or_rules->first;
	_c++;
	_c->or_rule = 0;
	_c->ref_rule = 0;
	_c->rule = 0;
}

void GrammarLoader::or()
{
	_c->or_rule = *_c->ref_or_rule = new GrammarOrRule;
	_c->ref_or_rule = &_c->or_rule->next;
	_c->ref_rule = &_c->or_rule->rule;
}

void GrammarLoader::close()
{
	_c--;
}

void GrammarLoader::rec_or()
{
	_c->or_rule = *_ref_rec_or_rule = new GrammarOrRule;
	_ref_rec_or_rule = &_c->or_rule->next;
	_c->ref_rule = &_c->or_rule->rule;
}

void GrammarLoader::tree(const char* name)
{
	_c->or_rule->tree_name = Ident(name);
}

void GrammarLoader::pos(int line, int column)
{
	_c->rule->line = line;
	_c->rule->column = column;
}

#ifdef IMPLEMENTED_GRAMMAR_LOADER_FOR_APT // work in progress
void GrammarLoaderFromAPT::load(const AbstractParseTree& root)
{
    for (AbstractParseTree::iterator rules(root); rules.more(); rules.next())
    {
    	AbstractParseTree rule(rules);
        if (rule.isTree(id_nt_def))
        {   
			nt_def(rule.part(1).identName());

            for (AbstractParseTree::iterator or_rule(rule.part(2)); or_rule.more();)
            {   AbstractParseTree rule(or_rule);
                if (rule.isTree(id_rule))
                {   GrammarOrRule* new_or_rule = new GrammarOrRule;

                    if (rule.nrParts() == 0)
                    {   
						or();
						or_rule.next();
                    }
                    else
                    {   AbstractParseTree::iterator parts(rule.part(1));
                        if (parts.more() && AbstractParseTree(parts).isIdent(nt_name))
                        {
                        	parts.next();
							rec_or();
							make_rule(parts);
							or_rule.next();
                        }
                        else
                        {
							or();
							int nr_combined_rules = 1;
							CombinedRule* combinedRules = 0;
							if (_for_unparse || !parts.more())
								or_rule.next();
							else
							{
								combinedRules = new CombinedRule(parts, tree_name);
								CombinedRule** ref_next = &combinedRules->next;
								for (;;)
								{
									or_rule.next();
									if (!or_rule.more())
										break;
									AbstractParseTree next_rule(or_rule);
									if (   next_rule.isTree(id_rule)
										&& next_rule.part(1).nrParts() > 1
										&& equivalent(parts, next_rule.part(1).part(0)))
									{
										nr_combined_rules++;
										Ident next_tree_name;
										if (next_rule.part(2).isIdent())
											next_tree_name = next_rule.part(2).stringValue();

										(*ref_next) = new CombinedRule(next_rule.part(1), next_tree_name);
										ref_next = &(*ref_next)->next;
									}
									else
										break;
								}
							}
							if (nr_combined_rules == 1)
								make_rule(parts);
							else
								new_or_rule->rule = make_rule(combinedRules);
							delete combinedRules;
                        }
						if (rule.part(2).isIdent())
							tree(rule.part(2).stringValue());
                    }                     
                }
            }
        }
    }
	if (false)
    {	GrammarNonTerminal* nt;
    
    	for (nt = _all_nt; nt != 0; nt = nt->next)
    		if (nt->first == 0 && nt->recursive == 0)
    		    printf("Non-terminal '%s' has no rule.\n", nt->name.val());
    		else
    		{
    			printf("Non-terminal '%s'\n", nt->name.val());
    			
    			for (GrammarOrRule* o_r = nt->first; o_r != 0; o_r = o_r->next)
    			{
    				printf(" First: ");
    				if (o_r->rule != 0)
    					o_r->rule->print(stdout);
    				printf("\n");
    			}
    			for (GrammarOrRule* o_r2 = nt->recursive; o_r2 != 0; o_r2 = o_r2->next)
    			{
    				printf(" Recursive: ");
    				if (o_r2->rule != 0)
    					o_r2->rule->print(stdout);
    				printf("\n");
    			}
    		}
    }		    
}

void GrammarLoaderForAPT::make_elem(AbstractParseTree elem)
{
    /* Is the first element optional? */
    if (elem.equalTree(id_opt))
    {   make_elem(elem.part(1));
		opt();
		if (elem.part(2).isTree(id_avoid))
			avoid();
		else if (elem.part(2).isTree(id_nongreedy))
			nongreedy();
		return;
    }

    /* Is it a sequential, chain or list? */
    if (elem.equalTree(id_seq))
    {   make_elem(elem.part(1));
		seq();
		if (elem.part(2).isTree(id_avoid))
			avoid();
		return;
    }
    if (elem.equalTree(id_chain))
    {   make_elem(elem.part(1));
        chain(elem.part(3).string());
		if (elem.part(2).isTree(id_avoid))
			avoid();
		return;
    }
    if (elem.equalTree(id_list))
    {   make_elem(elem.part(1));
		chain(",");
		if (elem.part(2).isTree(id_avoid))
			avoid();
		return;
    }

    if (elem.isIdent())
    {
       	Ident name = elem.identName();
       	
        if (name == id_eof)
            eof();
        else if (findNonTerminal(name))
			nt(name);
		else
			term(name);
    }
	else if (elem.isTree(id_wsterminal))
	{
		Ident name = elem.part(1).identName();
		if (findNonTerminal(name))
			ws_nt(name);
		else
			ws(name);
	}
    else if (elem.isTree(id_identalone))
    {
		term(id_ident);
    }
    else if (elem.isTree(id_identdef))
    {
        printf("Not supported\n"); //result->kind = RK_IDENT;
        //result->text.ident = new GrammarIdent(IK_IDENTDEF, elem.part(2).string(), findTerminal(id_ident));
    }
    else if (elem.isTree(id_identdefadd))
    {
        printf("Not supported\n"); //result->kind = RK_IDENT;
        //result->text.ident = new GrammarIdent(IK_IDENTDEFADD, elem.part(2).string(), findTerminal(id_ident));
    }
    else if (elem.isTree(id_identuse))
    {
        printf("Not supported\n"); //result->kind = RK_IDENT;
        //result->text.ident = new GrammarIdent(IK_IDENTUSE, elem.part(2).string(), findTerminal(id_ident));
    }
    else if (elem.isTree(id_identfield))
    {
        printf("Not supported\n"); //result->kind = RK_IDENT;
        //result->text.ident = new GrammarIdent(IK_IDENTFIELD, elem.part(2).string(), findTerminal(id_ident));
    }
    else if (elem.isTree(id_opencontext))
    {
    	printf("Not supported\n"); //result->kind = RK_T_OPENCONTEXT;
    }
    else if (elem.isTree(id_closecontext))
    {
		printf("Not supported\n"); //result->kind = RK_T_CLOSECONTEXT;
	}
    else if (elem.isTree(id_literal))
		lit(elem.part(1).string(), elem.part(2).isTree(id_local));
	else if (elem.isTree(id_charset))
		make_char_set(elem.part(1), /*avoid*/false);
	else if (elem.isTree(id_avoid))
		make_char_set(elem.part(1), /*avoid*/true);
	else if (elem.isTree(id_avoidlit))
		avoid_lit(elem.part(1).string();
	else if (elem.isTree(id_colourcommand))
		colour_coding(elem.part(1).charValue(), elem.part(2).intValue(), elem.part(3).intValue());
    else if (elem.isList())
		make_or_rule(elem);
	else
		printf("Error\n");
	
	update_single_element();
	/*
	if (result->kind == RK_TERM || result->kind == RK_NT || result->kind == RK_OR_RULE)
	{
		or_rule->single_element = ++or_rule->nr_active == 1 ? result : 0;
	}*/
}

void GrammarLoaderForAPT::make_rule(AbstractParseTree::iterator rule)
{
	GrammarRule* result;
 
	for (rule.more(); rule.next())
	{
		make_elem(rule);
		pos(rule.line(), rule.column());
	}
}

void GrammarLoaderForAPT::make_char_set(AbstractParseTree char_set_rule)
{
	char_set();
	for (AbstractParseTree::iterator it(char_set_rule.part(1)); it.more(); it.next())
	{
		AbstractParseTree part(it);
		if (part.isChar())
			add_char(part.charValue());
		else if (part.isTree(id_charrange))
			add_range(part.part(1).charValue(), part.part(2).charValue());
		else if (part.isTree(id_any))
			add_any();
	}
	for (AbstractParseTree::iterator it(char_set_rule.part(2)); it.more(); it.next())
	{
		AbstractParseTree part(it);
		remove_char(part.charValue());
	}
}

void GrammarLoaderForAPT::make_rule(CombinedRule* rules)
{
	GrammarRule* result;

	bool all_equivalent = rules->rule.more();
	for (CombinedRule* rule = rules->next; rule != 0 && all_equivalent; rule = rule->next)
		if (!rule->rule.more() || !equivalent(rules->rule, rule->rule))
			all_equivalent = false;

    result = new GrammarRule;
	if (all_equivalent)
	{
		AbstractParseTree elem = rules->rule;
		if (elem.isIdent())
		{
       		Ident name = elem.identName();
	       	
			if (name == id_eof)
				result->kind = RK_T_EOF;
			else
			{
				GrammarNonTerminal* nt = findNonTerminal(name);
        		if (nt != 0)
        		{
					result->kind = RK_NT;
					result->text.non_terminal = nt;
				}
				else
        		{
        			result->kind = RK_TERM;
        			result->text.terminal = new GrammarTerminal(name);
        		}
			}
		}
		else if (elem.isTree(id_wsterminal))
		{
			result->kind = RK_WS_TERM;
			result->text.terminal = new GrammarTerminal(elem.part(1).identName());
		}
		else if (elem.isTree(id_identalone))
		{
			result->kind = RK_TERM;
			result->text.terminal = new GrammarTerminal(id_ident);
		}

		for (CombinedRule* rule = rules; rule != 0; rule = rule->next)
			rule->rule.next();

		result->next = make_rule(rules);
	}
	else
	{
		// One deep implementation
		result->kind = RK_COR_RULE;
		result->text.or_rules = new GrammarOrRules;
		GrammarOrRule** ref_last = &result->text.or_rules->first;
		for (CombinedRule* rule = rules; rule != 0; rule = rule->next)
		{
			GrammarOrRule *new_or_rule = new GrammarOrRule;
			new_or_rule->tree_name = rule->tree_name;
			new_or_rule->rule = make_rule(rule->rule, new_or_rule);
			(*ref_last) = new_or_rule;
			ref_last = &(*ref_last)->next;
		}
	}

	return result;
}

bool GrammarLoaderForAPT::equivalent(const AbstractParseTree& lhs, const AbstractParseTree& rhs)
{
	if (lhs.isIdent())
		return rhs.isIdent() && lhs.identName() == rhs.identName();
	if (rhs.isIdent())
		return false;
	if (lhs.isTree(id_wsterminal))
		return rhs.isTree(id_wsterminal) && lhs.part(1).identName() == rhs.part(1).identName();
	if (rhs.isTree(id_wsterminal))
		return false;
	if (lhs.isTree(id_identalone))
		return rhs.isTree(id_identalone);

	return false;
}

void GrammarLoaderForAPT::make_or_rule(AbstractParseTree elem)
{
	open();
	for (AbtractParseTree::iterator it(elem); it.more(); it.next())
    {   AbstractParseTree rule = or_rule;
        if (rule.isTree(id_rule))
			make_rule(rule);
        }
    }
	close();
}

#endif

