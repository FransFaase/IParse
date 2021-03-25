#ifndef _INCLUDED_UNPARSER_H
#define _INCLUDED_UNPARSER_H

#include "ParserGrammar.h"
#include "Streams.h"

class AbstractTerminalUnparser
{
public:
	virtual void init(AbstractStream<char> *outputStream) = 0;
	virtual bool match(Ident terminal, const AbstractParseTree& tree) = 0;
	virtual void unparse(Ident terminal, const AbstractParseTree& tree) = 0;
	virtual void unparseLiteral(const char* literal) = 0;
	virtual void unparseLiteral(const char* literal, const AbstractParseTree& tree) { unparseLiteral(literal); }
	virtual void unparseWhiteSpace(Ident terminal) = 0;
};

class BasicTerminalUnparser : public AbstractTerminalUnparser
{
public:
	BasicTerminalUnparser() : _stream('\n'), _indent(0), _state('\0') {}
	
	virtual void init(AbstractStream<char> *outputStream);
	virtual bool match(Ident terminal, const AbstractParseTree& tree);
	virtual void unparse(Ident terminal, const AbstractParseTree& tree);
	virtual void unparseLiteral(const char* literal);
	virtual void unparseWhiteSpace(Ident terminal);
protected:
	LineColTrackerStream<char>  _stream;
	int _indent;
	char _state;
};

class WhiteSpaceTerminalUnparser : public BasicTerminalUnparser
{
public:
	virtual bool match(Ident terminal, const AbstractParseTree& tree);
	virtual void unparse(Ident terminal, const AbstractParseTree& tree);
	virtual void unparseLiteral(const char* literal);
	virtual void unparseWhiteSpace(Ident terminal);
};

class ResourceTerminalUnparser : public AbstractTerminalUnparser
{
public:
	ResourceTerminalUnparser() : _stream(/*text=*/true), _indent(0), _state('\0') {}
	
	virtual void init(AbstractStream<char> *outputStream);
	virtual bool match(Ident terminal, const AbstractParseTree& tree);
	virtual void unparse(Ident terminal, const AbstractParseTree& tree);
	virtual void unparseLiteral(const char* literal);
	virtual void unparseWhiteSpace(Ident terminal);
private:
	UTF8LineColTrackerStream _stream;
	int _indent;
	char _state;
};

class MarkDownCTerminalUnparser : public BasicTerminalUnparser
{
public:
	MarkDownCTerminalUnparser(bool with_line_numbers) : BasicTerminalUnparser(), _filename(NULL), _line(0), _column(1), _with_line_numbers(with_line_numbers) {}
	virtual bool match(Ident terminal, const AbstractParseTree& tree);
	virtual void unparse(Ident terminal, const AbstractParseTree& tree);
	virtual void unparseLiteral(const char* literal, const AbstractParseTree& tree);
	virtual void unparseWhiteSpace(Ident terminal);
private:
	void moveToLineAndColumn(const AbstractParseTree& tree);
	bool _with_line_numbers;
	const char* _filename;
	int _line;
	int _column;
};

class Unparser : public Grammar
{
public:
	void setTerminalUnparser(AbstractTerminalUnparser* terminalUnparser) { _terminalUnparser = terminalUnparser; }
	
	void unparse(const AbstractParseTree& tree, Ident root_id, AbstractStream<char> *outputStream);

protected:
	void unparse_term(const AbstractParseTree& tree, Ident name);
	void unparse_ident(const AbstractParseTree& tree, GrammarIdent* ident);
	bool match_or(const AbstractParseTree& tree, GrammarOrRules* or_rules, bool nt_with_recursive = false);
	bool match_rule_elem(const AbstractParseTree& part, GrammarRule* rule);
	bool match_rule(AbstractParseTree::iterator partIt, GrammarRule* rule);
	bool match_rule_single(const AbstractParseTree& part, GrammarRule* rule);
	bool match(const AbstractParseTree& tree, TreeTypeToGrammarRule *treeTypeToRule);
	void unparse_or(const AbstractParseTree& tree, GrammarOrRules* or_rules, bool nt_with_recursive = false);
	void unparse_rule_elem(const AbstractParseTree& part, GrammarRule* rule);
	void unparse_rule(const AbstractParseTree& tree, AbstractParseTree::iterator partIt, GrammarRule* rule);
	void unparse_rule_single(const AbstractParseTree& part, GrammarRule* rule);

	AbstractTerminalUnparser* _terminalUnparser;
};



#endif // _INCLUDED_UNPARSER_H

