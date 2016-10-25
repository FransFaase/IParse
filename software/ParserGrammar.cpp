#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Ident.h"
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

Ident TreeTypeToGrammarRules::id_empty_tree = "<empty>";
Ident TreeTypeToGrammarRules::id_list = "<list>";

void GrammarRule::print(FILE *f)
{   
    if (this == 0)
        return;

	fprintf(f, "[%d.%d]", line, column); 
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
            text.or_rules->first->print(f);
            fprintf(f, ")");
            break;
        case RK_COR_RULE:
            fprintf(f, "C(");
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

    next->print(f);
}

void GrammarOrRule::print(FILE *f, bool first)
{
	if (this == 0)
		return;
	if (!first)
		fprintf(f, "|");
    rule->print(f);
    if (!tree_name.empty())
       fprintf(f, "[%s]", tree_name.val());
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
        result->text.ident = new GrammarIdent(IK_IDENTDEF, elem.part(2).string(), findTerminal(id_ident));
    }
    else if (elem.isTree(id_identdefadd))
    {
        result->kind = RK_IDENT;
        result->text.ident = new GrammarIdent(IK_IDENTDEFADD, elem.part(2).string(), findTerminal(id_ident));
    }
    else if (elem.isTree(id_identuse))
    {
        result->kind = RK_IDENT;
        result->text.ident = new GrammarIdent(IK_IDENTUSE, elem.part(2).string(), findTerminal(id_ident));
    }
    else if (elem.isTree(id_identfield))
    {
        result->kind = RK_IDENT;
        result->text.ident = new GrammarIdent(IK_IDENTFIELD, elem.part(2).string(), findTerminal(id_ident));
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
			
			*ref_nt = new GrammarNonTerminal(nt_name);
			ref_nt = &(*ref_nt)->next;
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
	if(false)
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
    				o_r->rule->print(stdout);
    				printf("\n");
    			}
    			for (GrammarOrRule* o_r2 = nt->recursive; o_r2 != 0; o_r2 = o_r2->next)
    			{
    				printf(" Recursive: ");
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

void processOrRules(GrammarOrRules *or_rules, GrammarRule *top_rule, GrammarNonTerminal *rec_nt, GrammarOrRule* or_rule, AbstractUnparseErrorCollector *unparseErrorCollector)
{
	if (!or_rule->tree_name.empty())
		addToTreeTypeToRulesMap(or_rule->tree_name, &or_rules->treeNameToRulesMap, rec_nt, or_rule, top_rule, unparseErrorCollector);
	else
	{
		GrammarRule* single_rule = or_rule->single_element; 
		if (rec_nt == 0 && (or_rule->rule == 0 || (single_rule != 0 && single_rule->optional)))
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

