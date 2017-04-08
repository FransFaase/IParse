#ifndef INCLUDED_ABSTRACTPARSETREE_H
#define INCLUDED_ABSTRACTPARSETREE_H

struct tree_t;
struct list_t;
struct tree_cursor_t;

class AbstractParseTree;
class AbstractParseTreeIteratorCursor;
class AbstractParseTreeCursor;
class AbstractParseTreeIteratorCursor;

class AbstractParseTreeBase
{
	friend struct tree_cursor_t;
public:
	AbstractParseTreeBase() : _tree(0), _cursor(0) {}

	bool isEmpty() const { return _tree == 0; }

	bool isIdent() const;
	bool isIdent( const Ident ident ) const;
	bool equalIdent( const Ident ident ) const;
	Ident identName() const;
	bool isString() const;
	bool isString( const char *str ) const;
	String string() const;
	const char* stringValue() const;
	bool isInt() const;
	long intValue() const;
	bool isDouble() const;
	double doubleValue() const;
	bool isChar() const;
	char charValue() const;
	bool isList() const;
	bool isTree() const;
	bool isTree( Ident name ) const;
	bool equalTree( Ident name ) const;
	const char* type() const;
	int nrParts() const;
	int line() const;
	int column() const;

	void print(FILE* f, bool compact) const;
	
protected:
	tree_t *_tree;
	tree_cursor_t *_cursor;
};

class AbstractParseTree : public AbstractParseTreeBase
{
	friend class AbstractParseTreeCursor;
	friend class AbstractParseTreeIteratorCursor;
public:
	AbstractParseTree() {}
	AbstractParseTree(const AbstractParseTree& lhs);
	AbstractParseTree(const Ident ident) { createIdent(ident); }
	AbstractParseTree(const char *str) { createStringAtom(str); }
	AbstractParseTree(String &str) { createStringAtom(str); }
	AbstractParseTree(double value) { createDoubleAtom(value); }
	AbstractParseTree(char value) { createCharAtom(value); }
	AbstractParseTree(long value) { createIntAtom(value); }
	AbstractParseTree& operator=(const AbstractParseTree& lhs);
	AbstractParseTree& operator=(const Ident ident) { createIdent(ident); return *this; }
	AbstractParseTree& operator=(const char *str) { createStringAtom(str); return *this; }
	AbstractParseTree& operator=(String &str) { createStringAtom(str); return *this; }
	AbstractParseTree& operator=(double value) { createDoubleAtom(value); return *this; }
	AbstractParseTree& operator=(char value) { createCharAtom(value); return *this; }
	AbstractParseTree& operator=(long value) { createIntAtom(value); return *this; }
	~AbstractParseTree() { release(); }

	static AbstractParseTree makeList();
	static AbstractParseTree makeTree( const Ident name );

	class iterator
	{
		friend class AbstractParseTree;
		friend class AbstractParseTreeCursor;
	public:
		iterator() : _list(0) {}
		iterator(const iterator& it) { _list = it._list; }
		iterator(const AbstractParseTree& tree);
		bool more();
		bool isTree(Ident name);
		void next();
		
	protected:
		iterator(tree_t *tree);
		list_t *_list;
	};
	
	friend class iterator;

	AbstractParseTree(const iterator& lhs);
	
	void clear();
	void attach(AbstractParseTree& tree);
	
	void createIdent( const Ident ident );
	void createStringAtom( const char *str );
	void createStringAtom( String &str );
	void createDoubleAtom( double value );
	void createCharAtom( char value );
	void createIntAtom( long value );
	void createList();
	void createTree( const Ident name );
	void setTreeName( const Ident name );
	void insertChild( const AbstractParseTree& child );
	void appendChild( const AbstractParseTree& child );
	void dropLastChild();
	void createOpenContext();
	void createCloseContext();
	
	AbstractParseTree part(int n) const;

	void setLineColumn(int line, int column);

private:
	AbstractParseTree(tree_t *tree);
	void release();
	void create();
};

class AbstractParseTreeCursor : public AbstractParseTreeBase
{
	friend class AbstractParseTreeIteratorCursor;
	friend struct tree_cursor_t;
public:
	AbstractParseTreeCursor();
	AbstractParseTreeCursor(AbstractParseTree &root);
	AbstractParseTreeCursor(const AbstractParseTreeIteratorCursor& it);
	AbstractParseTreeCursor(const AbstractParseTreeCursor& cursor);
	~AbstractParseTreeCursor();

	bool attached() const { return _cursor != 0; }

	operator AbstractParseTree() const;
	AbstractParseTreeCursor part(int n);

	AbstractParseTreeCursor& operator=(const AbstractParseTreeCursor& cursor);
	AbstractParseTreeCursor& operator=(const AbstractParseTreeIteratorCursor &it);

	void replaceBy(AbstractParseTree tree);

private:
	void attach();
	void detach();
	AbstractParseTreeCursor **_ref_prev;
	AbstractParseTreeCursor *_next;
};

class AbstractParseTreeIteratorCursor : public AbstractParseTree::iterator
{
	friend class AbstractParseTreeCursor;
	friend struct tree_cursor_t;
public:
	AbstractParseTreeIteratorCursor(AbstractParseTreeCursor& tree);
	~AbstractParseTreeIteratorCursor();
	void next();

	void erase();
	void insert(const AbstractParseTree child);

	bool attached() const { return _parent != 0; }
	
private:
	tree_cursor_t *_parent;
	int _part_nr;
	AbstractParseTreeIteratorCursor **_ref_prev;
	AbstractParseTreeIteratorCursor *_next;
};

#ifdef _DEBUG
void AbstractParseTreeUnitTest();
#endif



#endif // INCLUDED_ABSTRACTPARSETREE_H
