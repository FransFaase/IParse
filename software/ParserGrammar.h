#ifndef _INCLUDED_PARSERGRAMMAR_H
#define _INCLUDED_PARSERGRAMMAR_H

class GrammarRule;

#define RK_NT              0
#define RK_LIT             1
#define RK_CHARSET		   2
#define RK_AVOID		   3
#define RK_AVOIDLIT		   4
#define RK_OR_RULE         5
#define RK_COR_RULE        6
#define RK_T_EOF           7
#define RK_TERM			   8
#define RK_WS_TERM		   9
#define RK_WS_NT          10
#define RK_IDENT          11
#define RK_T_OPENCONTEXT  12
#define RK_T_CLOSECONTEXT 13
#define RK_COLOURCODING	  14

#define IK_IDENTDEF      1
#define IK_IDENTDEFADD   2
#define IK_IDENTUSE      3
#define IK_IDENTFIELD    4

class GrammarOrRule
{   
public:
	GrammarOrRule() : next(0), rule(0), nr_active(0), single_element(0) {}
	~GrammarOrRule();
	void print(FILE* fout, bool first = true);
	GrammarOrRule* next;
    GrammarRule* rule;
    Ident tree_name;
	int nr_active;
	GrammarRule* single_element;
};

class TreeTypeToGrammarRules;

class GrammarOrRules
{
public:
	GrammarOrRules() : first(0), emptyTreeRules(0), treeNameToRulesMap(0), listRules(0), terminalToRulesMap(0) {}
	~GrammarOrRules();
	GrammarOrRule* first;
	TreeTypeToGrammarRules* emptyTreeRules;
	TreeTypeToGrammarRules* treeNameToRulesMap;
	TreeTypeToGrammarRules* listRules;
	TreeTypeToGrammarRules* terminalToRulesMap;
};

class GrammarNonTerminal : public GrammarOrRules
{
public:
	GrammarNonTerminal(Ident n_name) : next(0), name(n_name), recursive(0) {}
	~GrammarNonTerminal();
	GrammarNonTerminal* next;
    Ident name;
    GrammarOrRule* recursive;
};

class GrammarTerminal
{	
public:
	GrammarTerminal(const Ident& new_name) : name(new_name), next(0) {}
	Ident name;
	GrammarTerminal *next;
};

class GrammarIdent
{
public:
	GrammarIdent(int n_kind, const String& n_ident_class, GrammarTerminal* n_terminal)
	 : kind(n_kind), ident_class(n_ident_class), terminal(n_terminal) {}
	int kind;
	String ident_class;
	GrammarTerminal* terminal;
};

class GrammarCharSet
{
public:
	GrammarCharSet();
	inline void add_char(unsigned char ch) { _range[ch >> 5] |= 1 << (ch & 0x01f); }
	void add_range(unsigned char from, unsigned char to);
	inline void remove_char(unsigned char ch) { _range[ch >> 5] &= ~(1 << (ch & 0x01f)); }
	inline bool contains_char(unsigned char ch) const { return (_range[ch >> 5] & (1 << (ch & 0x01f))) != 0; }
	class RangeIterator
	{
	public:
		RangeIterator(GrammarCharSet& char_set) : _char_set(char_set), _ch(0) { next(); }
		inline bool more() { return _more; }
		inline int from() { return _from; }
		inline int to() { return _to; }
		void next();
	private:
		GrammarCharSet& _char_set;
		bool _more;
		int _ch;
		int _from;
		int _to;
	};
private:
	long _range[8];
};

class GrammarColourCoding
{
public:
	GrammarColourCoding(char type, long fg, long bg) : _type(type), _fg(fg), _bg(bg) {}
	inline char type() const { return _type; }
	inline long fg() const { return _fg; }
	inline long bg() const { return _bg; }
private:
	char _type;
	long _fg;
	long _bg;
};

class GrammarRule
{
public:
	GrammarRule() : next(0), optional(false), sequential(false), avoid(false), nongreedy(false), last_fail_pos(-1) {}
	~GrammarRule();
	void print(FILE* fout);
	GrammarRule* next;
    bool optional;
    bool sequential;
	bool avoid;
	bool nongreedy;
    String chain_symbol;
    int kind;
    union 
    {   GrammarNonTerminal* non_terminal;
    	GrammarTerminal* terminal;    
        GrammarOrRules* or_rules;
        GrammarIdent* ident;
		GrammarCharSet* char_set;
		GrammarColourCoding* colour_coding;
    } text;
    String str_value;
	// position grammar text
	long line;
	long column;
    size_t last_fail_pos; // Used by back-tracking parsers
};

class TreeTypeToGrammarRule
{
public:
	TreeTypeToGrammarRule(GrammarRule* n_top_rule, GrammarNonTerminal *n_rec_nt, GrammarOrRule* n_or_rule)
	  : top_rule(n_top_rule), rec_nt(n_rec_nt), 
		is_single(n_or_rule->tree_name.empty() && n_rec_nt == 0 && n_or_rule->single_element != 0), 
		rule(n_or_rule->rule), next(0) {}
	~TreeTypeToGrammarRule();
	GrammarRule* top_rule;
	GrammarNonTerminal *rec_nt;
	bool is_single;
	GrammarRule* rule;
	TreeTypeToGrammarRule* next;
};

class TreeTypeToGrammarRules
{
public:
	TreeTypeToGrammarRules(Ident n_name) : name(n_name), nr_alternatives(0), alternatives(0), next(0) {}
	~TreeTypeToGrammarRules();
	void add(TreeTypeToGrammarRule *alternative)
	{
		nr_alternatives++;
		TreeTypeToGrammarRule **ref_alternative = &alternatives;
		while ((*ref_alternative) != 0)
			ref_alternative = &(*ref_alternative)->next;
		*ref_alternative = alternative;
	}
	Ident name;
	int nr_alternatives;
	TreeTypeToGrammarRule* alternatives;
	TreeTypeToGrammarRules* next;

	static Ident id_empty_tree;
	static Ident id_list;
};


class AbstractUnparseErrorCollector
{
public:
	virtual void errorDifferentRulesWithSameType(Ident type, GrammarRule* rule1, GrammarRule* rule2) = 0;
	virtual void warningTypeReachedThroughDifferentPaths(Ident type, GrammarRule* rule, GrammarRule *rule1, GrammarRule *rule2) = 0;

};

class GrammarLiteral;
class CodeGenerator;

class Grammar
{
public:
	Grammar() : _all_nt(0), _all_t(0), _all_l(0), _for_unparse(false) {}
	virtual ~Grammar();
	void loadGrammar(const AbstractParseTree& root);
	void loadGrammarForUnparse(const AbstractParseTree& root, AbstractUnparseErrorCollector *unparseErrorCollector);
	GrammarNonTerminal* findNonTerminal(Ident name);
	GrammarNonTerminal* addNonTerminal(Ident name);
	GrammarTerminal* findTerminal(Ident name);
	void addLiteral(Ident literal);
	bool isLiteral(Ident literal);
	void outputGrammarAsCode(FILE* fout, const char* name);

private:
	class CombinedRule
	{
	public:
		CombinedRule(const AbstractParseTree::iterator& n_rule, Ident& n_tree_name) : rule(n_rule), tree_name(n_tree_name), next(0) { rule = n_rule; }
		~CombinedRule() { delete next; }
		AbstractParseTree::iterator rule;
		Ident tree_name;
		CombinedRule* next;
	};
	GrammarRule* make_rule(AbstractParseTree::iterator rule, GrammarOrRule* or_rule);
	GrammarRule* make_rule(CombinedRule* rules);
	bool equivalent(const AbstractParseTree& lhs, const AbstractParseTree& rhs);
	GrammarOrRule* make_or_rule(AbstractParseTree::iterator or_rule);
	void make_char_set(AbstractParseTree char_set_rule, GrammarCharSet *char_set);
	GrammarNonTerminal* _all_nt;
	GrammarTerminal* _all_t;
	GrammarLiteral* _all_l;
	bool _for_unparse;
	void outputCodeFor(GrammarOrRule* or_rule, CodeGenerator &codeGenerator);
};

class GrammarLoader
{
public:
	GrammarLoader(Grammar *grammar) : _nt(0), _c(0), _grammar(grammar) {}

protected:
	void nt_def(const char* name);
	void nt(const char* name);
	void term(const char* name);
	void lit(const char* sym, bool local = false);
	void ws_nt(const char* name);
	void ws(const char* name);
	void charset();
	void add_char(unsigned char ch);
	void add_range(unsigned char from, unsigned char to);
	void add_any();
	void remove_char(unsigned char ch);
	void eof();
	void chain(const char* sym);
	void seq();
	void opt();
	void avoid();
	void nongreedy();
	void open(bool combined = false);
	void or();
	void close();
	void rec_or();
	void tree(const char* name);
	void pos(int line, int column);

private:
	void _new_elem();
	GrammarNonTerminal* _nt;
	struct Context
	{
		Context() : ref_or_rule(0), ref_rule(0) {}
		GrammarOrRule** ref_or_rule;
		GrammarOrRule* or_rule;
		GrammarRule** ref_rule;
		GrammarRule* rule;
	};
	Context _contexts[50];
	Context* _c;
	GrammarOrRule** _ref_rec_or_rule;
	Grammar *_grammar;
};

//#define IMPLEMENTED_GRAMMAR_LOADER_FOR_APT
#ifdef IMPLEMENTED_GRAMMAR_LOADER_FOR_APT // work in progress
class GrammarLoaderForAPT : public GrammarLoader
{
pubic:
	GrammarLoader(Grammar* grammar) : GrammarLoader(grammar) {}
	void load(const AbstractParseTree& root);
	void loadForUnparse(const AbstractParseTree& root);
private:
	void make_rule(AbstractParseTree::iterator rule);
	void make_rule(CombinedRule* rules);
	bool equivalent(const AbstractParseTree& lhs, const AbstractParseTree& rhs);
	void make_or_rule(AbstractParseTree::iterator or_rule);
	void make_char_set(AbstractParseTree char_set_rule, GrammarCharSet *char_set);
};
#endif


#endif // _INCLUDED_PARSERGRAMMAR_H

