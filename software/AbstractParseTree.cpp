#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "Ident.h"
#include "String.h"
#include "malloc.h"
#include "AbstractParseTree.h"

#include "string_t.h"

struct tree_t
{   
	tree_t(const char* n_type) : type(n_type), filename(0), line(0), column(0), refcount(1) { c.parts = 0; }
	tree_t(const Ident ident) : type(tt_ident), filename(0), line(0), column(0), refcount(1) { c.ident = ident.val(); }
	tree_t(string_t *value) : type(tt_str_value), filename(0), line(0), column(0), refcount(1) { c.str_value = value; if (value != 0) value->refcount++; }
	tree_t(long value) : type(tt_int_value), filename(0), line(0), column(0), refcount(1) { c.int_value = value; }
	tree_t(double value) : type(tt_double_value), filename(0), line(0), column(0), refcount(1) { c.double_value = value; }
	tree_t(char value) : type(tt_char_value), filename(0), line(0), column(0), refcount(1) { c.char_value = value; }
	
	void release();
	
	bool can_have_parts();
	static void assign(tree_t *&d, tree_t *s);
	tree_t* clone();

	void setFileName(string_t *fn);
	void print(FILE *f, bool compact);

	const char *type;
	union
	{   list_t *parts;
		const char *ident;
		string_t *str_value;
		long   int_value;
		double double_value;
		char   char_value;
	} c;
	string_t *filename;
	int line, column;
	unsigned long refcount;


	static const char *tt_ident;
	static const char *tt_str_value;
	static const char *tt_int_value;
	static const char *tt_double_value;
	static const char *tt_char_value;
	static const char *tt_list;
	static const char *tt_opencontext;
	static const char *tt_closecontext;

#ifdef USE_OWN_NEW_DELETE
	void* operator new(size_t size); 
	void operator delete(void *data);
#endif
	static void Dispose();
	static long alloced;
private:
	static tree_t *_old;
};

struct list_t
{   
	list_t() : first(0), next(0) {}
	list_t(Ident n_name, tree_t *n_first) : name(n_name), first(n_first), next(0) {}
	Ident name;
	tree_t *first;
	list_t *next;

#ifdef USE_OWN_NEW_DELETE
	void* operator new(size_t size); 
	void operator delete(void *data);
#endif
	static void Dispose();
private:
	static list_t *_old;
};



const char *tree_t::tt_ident = "identifier";
const char *tree_t::tt_str_value = "string value";
const char *tree_t::tt_int_value = "integer value";
const char *tree_t::tt_double_value = "double value";
const char *tree_t::tt_char_value = "char value";
const char *tree_t::tt_list = "list";
const char *tree_t::tt_opencontext = "<opencontext>";
const char *tree_t::tt_closecontext = "<closecontext>";


tree_t *tree_t::_old = 0;
long tree_t::alloced = 0;

#ifdef USE_OWN_NEW_DELETE
void* tree_t::operator new(size_t size)
{   
	alloced++;
	if (_old == 0)
		return malloc(size);
	tree_t *new_tree = _old;
	_old = (tree_t*)_old->type;

	//printf("%8X +A ", new_tree);
	//SContext::print(stdout);
	//printf("\n");

	return new_tree;
}
void tree_t::operator delete(void *data)
{
	tree_t *tree = (tree_t*)data;
	tree->type = (char*)_old;
	_old = tree;
	alloced--;
}
#endif

void tree_t::Dispose()
{
	while (_old != 0)
	{
		tree_t *old = _old;
		_old = (tree_t*)_old->type;
		free(old);
	}
}


bool tree_t::can_have_parts()
{
	return    type != tt_ident 
		   && type != tt_str_value
		   && type != tt_int_value
		   && type != tt_double_value
		   && type != tt_char_value
		   && type != tt_opencontext
		   && type != tt_closecontext;
}

void tree_t::release()
{
	SCONTEXT("tree_t_release")
	//printf("%8X -- (%lu) ", this, refcount-1);
	//SContext::print(stdout);
	//printf("\n");

	alloced--;

	if (--refcount == 0)
	{
		if (type == tt_str_value)
		{
			delete c.str_value;
		}
		else if (can_have_parts())
		{   list_t *list = c.parts;

			while (list != 0)
			{   list_t *next = list->next;
				if (list->first != 0)
					list->first->release();
				delete list;
				list = next;
			}
		}
		if (filename != 0 && --filename->refcount == 0)
			delete filename;
			
		delete this;
	}
}

void tree_t::setFileName(string_t *fn)
{
	string_t* old_filename = filename;
	filename = fn;
	if (filename != 0)
		filename->refcount++;
	if (old_filename != 0 && --old_filename->refcount == 0)
		delete old_filename;
	if (can_have_parts())
	{
		for (list_t *list = c.parts; list != 0; list = list->next)
			if (list->first != NULL)
				list->first->setFileName(fn);
	}
}

void print_char(FILE *f, char ch, bool in_string)
{
	if (ch == '\0')
		fprintf(f, "\\0");
	else if (ch == '\n')
		fprintf(f, "\\n");
	else if (ch == '\r')
		fprintf(f, "\\r");
	else if (ch == '\t')
		fprintf(f, "\\t");
	else if (ch == '\'' && !in_string)
		fprintf(f, "\\'");
	else if (ch == '\"' && in_string)
		fprintf(f, "\\\"");
	else if (ch == '\\')
		fprintf(f, "\\\\");
	else if (ch < ' ' || ch >= 127)
		fprintf(f, "\\x%02x", (unsigned int)ch);
	else
		fprintf(f, "%c", ch);
}

void tree_t::print(FILE *f, bool compact)
{
	static int print_tree_depth = 0;
	if (line != 0)
	{
		if (filename != 0)
			fprintf(f, "<%s:%d:%d>", filename->value, line, column);
		else
			fprintf(f, "<%d:%d>", line, column);
	}
	if (type == tt_ident)
		fprintf(f, "%s", c.ident);
	else if (type == tt_str_value)
	{
		fprintf(f, "\"");
		for (const char *s = c.str_value != 0 ? c.str_value->value : ""; *s != '\0'; s++)
			print_char(f, *s, true);
		fprintf(f, "\"");
	}
	else if (type == tt_int_value)
		fprintf(f, "%ld", c.int_value);
	else if (type == tt_double_value)
		fprintf(f, "%f", c.double_value);
	else if (type == tt_char_value)
	{
		fprintf(f, "'");
		print_char(f, c.char_value, false);
		fprintf(f, "'");
	}
	else if (type == tt_opencontext)
	{   fprintf(f, "{");
		//print_context(f, tree->c.context, compact);
	}
	else if (type == tt_closecontext)
		fprintf(f, "}");
	else
	{
		fprintf(f, "%s(", type);
   
		print_tree_depth += strlen(type) + 1;

		bool first = true;
		for (list_t *list = c.parts; list != 0; list = list->next)
		{   
			if (!first)
			{   if (compact)
					fprintf(f, ",");
				else 
					fprintf(f, ",\n%*s", print_tree_depth, "");
			}
			else if (!compact)
				fprintf(f, "\n%*s", print_tree_depth, "");
			first = false;
			/* fprintf(f, "[%lx]", (longword)l); */
			if (list->first == 0)
				fprintf(f, "[EMPTY]");
			else
				list->first->print(f, compact);
		}

		print_tree_depth -= strlen(type) + 1;
		fprintf(f, ")");
	}
}

list_t *list_t::_old = 0;

#ifdef USE_OWN_NEW_DELETE
void* list_t::operator new(size_t size)
{   
	if (_old == 0)
		return malloc(size);
	list_t *new_list = _old;
	_old = _old->next;

	return new_list;
}
void list_t::operator delete(void *data)
{
	list_t *list = (list_t*)data;
	list->next = _old;
	_old = list;
}
#endif

void list_t::Dispose()
{
	while (_old != 0)
	{
		list_t *old = _old;
		_old = _old->next;
		free(old);
	}
}

void tree_t::assign(tree_t *&d, tree_t *s)
{
  SCONTEXT("tree_t_assign")
  tree_t *old_d = d;

  d = s;
  if (d != 0)
  {
	d->refcount++;
	alloced++;

	//printf("%8X +B ", d);
	//SContext::print(stdout);
	//printf("\n");
  }
  if (old_d != 0)
	old_d->release();
}

tree_t *tree_t::clone()
{
	SCONTEXT("tree_t_clone")

	tree_t *result = 0;
	if (type == tt_ident)
		result = new tree_t(c.ident);
	else if (type == tt_str_value)
		result = new tree_t(c.str_value);
	else if (type == tt_int_value)
		result = new tree_t(c.int_value);
	else if (type == tt_double_value)
		result = new tree_t(c.double_value);
	else if (type == tt_char_value)
		result = new tree_t(c.char_value);
	else
	{
		result = new tree_t(type);
		list_t **ref_list = &result->c.parts;
		*ref_list = 0;
		for (list_t *list = c.parts; list != 0; list = list->next)
		{
			*ref_list = new list_t(list->name, list->first);
			if (list->first != 0)
			{
				list->first->refcount++;
			}
			ref_list = &(*ref_list)->next;
		}
	}
	result->line = line;
	result->column = column;
	result->filename = filename;
	if (filename != 0)
		filename->refcount++;
	return result;
}

struct tree_cursor_t
{
	tree_cursor_t()
	  : root(0), parent(0), part_nr(0), ref_prev(0), next(0), cursors(0), iterator_cursors(0), child_cursors(0), tree(0) {}
	tree_cursor_t(AbstractParseTree& n_root, tree_t *n_tree)
	  : root(&n_root), parent(0), part_nr(0), cursors(0), iterator_cursors(0), child_cursors(0), tree(n_tree)
	{
		attach(n_root._cursor);
	}
	tree_cursor_t(tree_cursor_t *n_parent, int n_part_nr, tree_t *n_tree)
	  : root(0), parent(n_parent), part_nr(n_part_nr), cursors(0), iterator_cursors(0), child_cursors(0), tree(n_tree)
	{
		attach(parent->child_cursors);
	}
	void attach(tree_cursor_t *&child_cursors)
	{
		next = child_cursors;
		if (next != 0)
			next->ref_prev = &next;
		ref_prev = &child_cursors;
		child_cursors = this;
	}
	tree_cursor_t *child_for_part(int part_nr, tree_t* tree)
	{
		// See if there is already a child cursor for the given part
		for (tree_cursor_t *cursor = child_cursors; cursor != 0; cursor = cursor->next)
			if (cursor->part_nr == part_nr)
				return cursor;

		// If not, create new child cursor with the parent
		return new tree_cursor_t(this, part_nr, tree);
	}
	void release()
	{
		if (cursors == 0 && iterator_cursors == 0 && child_cursors == 0)
		{
			if (ref_prev != 0)
			{
				*ref_prev = next;
				if (next != 0)
					next->ref_prev = ref_prev;
				if (parent != 0)
					parent->release();
			}
			delete this;
		}
	}
	void assign(tree_t *new_tree);
	bool make_private_copy();
	void detach();
	void detach_children();

	AbstractParseTree *root;
	tree_cursor_t *parent;
	int part_nr;
	tree_cursor_t **ref_prev;
	tree_cursor_t *next;
	AbstractParseTreeCursor *cursors;
	AbstractParseTreeIteratorCursor *iterator_cursors;
	tree_cursor_t *child_cursors;
	tree_t *tree;

	void* operator new(size_t size); 
	void operator delete(void *data);
	static void Dispose();
	static long alloced;
private:
	static tree_cursor_t *_old;
};

tree_cursor_t *tree_cursor_t::_old = 0;
long tree_cursor_t::alloced = 0;

void* tree_cursor_t::operator new(size_t size)
{
	if (_old == 0)
		return malloc(size);

	tree_cursor_t *new_tree_cursor = _old;
	_old = _old->parent;

	alloced++;

	//printf("%8X +A ", new_tree_cursor);
	//SContext::print(stdout);
	//printf("\n");

	return new_tree_cursor;
}
void tree_cursor_t::operator delete(void *data)
{
	tree_cursor_t *tree_cursor = (tree_cursor_t*)data;
	tree_cursor->parent = _old;
	_old = tree_cursor;
}

void tree_cursor_t::Dispose()
{
	while (_old != 0)
	{
		tree_cursor_t *old = _old;
		_old = _old->parent;
		free(old);
	}
}

void tree_cursor_t::detach()
{
	while (cursors != 0)
	{
		AbstractParseTreeCursor *cursor = cursors;
		cursors = cursors->_next;
		cursor->_cursor = 0;
		cursor->_ref_prev = 0;
		cursor->_next = 0;
		cursor->_tree = 0;
	}

	detach_children();

	release();
}

void tree_cursor_t::detach_children()
{
	while (child_cursors != 0)
	{
		tree_cursor_t* child = child_cursors;
		child_cursors = child_cursors->next;
		child->root = 0;
		child->parent = 0;
		child->ref_prev = 0;
		child->next = 0;
		child->tree = 0;

		child->detach();
	}

	while (iterator_cursors != 0)
	{
		AbstractParseTreeIteratorCursor *iterator_cursor = iterator_cursors;
		iterator_cursor = iterator_cursor->_next;
		iterator_cursor->_parent = 0;
		iterator_cursor->_ref_prev = 0;
		iterator_cursor->_next = 0;
		iterator_cursor->_list = 0;
	}
}

void tree_cursor_t::assign(tree_t *new_tree)
{
	tree_t *old_tree = tree;

	// hook new clone into parent
	if (root != 0)
		root->_tree = new_tree;
	else if (parent != 0 && parent->tree != 0) // should be true
	{
		list_t *list = parent->tree->c.parts;
		for (int i = 1; i < part_nr; i++)
			list = list->next;
		list->first = new_tree;
	}
	tree = new_tree;

	// fix all cursors
	for (AbstractParseTreeCursor *cursor = cursors; cursor != 0; cursor = cursor->_next)
		cursor->_tree = tree;

	if (old_tree != 0)
		old_tree->release();
}

bool tree_cursor_t::make_private_copy()
{
	if ((parent == 0 || !parent->make_private_copy()) && (tree == 0 || tree->refcount == 1))
		return false;

	assign(tree == 0 ? 0 : tree->clone());

	// fix all cursor iterators
	for (AbstractParseTreeIteratorCursor *iterator_cursor = iterator_cursors; iterator_cursor != 0; iterator_cursor = iterator_cursor->_next)
	{
		list_t *list = tree->c.parts;
		for (int i = 1; i < iterator_cursor->_part_nr; i++)
			list = list->next;
		iterator_cursor->_list = list;
	}
	
	return true;
}


// AbstractParseTreeBase

bool AbstractParseTreeBase::isIdent() const
{
	return _tree != 0 && _tree->type == tree_t::tt_ident;
}

bool AbstractParseTreeBase::isIdent( const Ident ident ) const
{
	return    _tree != 0
		   && _tree->type == tree_t::tt_ident
		   && _tree->c.ident == ident.val();
}

bool AbstractParseTreeBase::equalIdent( const Ident ident ) const
{
	return _tree->c.ident == ident.val();
}

const Ident AbstractParseTreeBase::identName() const
{
	return _tree->c.ident;
}

bool AbstractParseTreeBase::isString( ) const
{
	return _tree != 0 && _tree->type == tree_t::tt_str_value;
}

bool AbstractParseTreeBase::isString( const char *str ) const
{
	return    _tree != 0
		   && _tree->type == tree_t::tt_str_value
		   && _tree->c.str_value != 0
		   && _tree->c.str_value->value == str;
}

const String AbstractParseTreeBase::string() const
{
	return _tree->c.str_value;
}
const char* AbstractParseTreeBase::stringValue() const
{
	if (_tree->type == tree_t::tt_ident)
		return _tree->c.ident;
	else
		return _tree->c.str_value != 0 ? _tree->c.str_value->value : "";
}

bool AbstractParseTreeBase::isInt() const
{
	return _tree != 0 && _tree->type == tree_t::tt_int_value;
}

long AbstractParseTreeBase::intValue() const
{
	return _tree->c.int_value;
} 

bool AbstractParseTreeBase::isDouble() const
{
	return _tree != 0 && _tree->type == tree_t::tt_double_value;
}

double AbstractParseTreeBase::doubleValue() const
{
	return _tree->c.double_value;
}

bool AbstractParseTreeBase::isChar() const
{
	return _tree != 0 && _tree->type == tree_t::tt_char_value;
}

char AbstractParseTreeBase::charValue() const
{
	return _tree->c.char_value;
}

bool AbstractParseTreeBase::isList() const
{
	return _tree != 0 && _tree->type == tree_t::tt_list;
}

bool AbstractParseTreeBase::isTree() const
{
	return    _tree != 0
		   && _tree->type != tree_t::tt_ident
		   && _tree->type != tree_t::tt_str_value
		   && _tree->type != tree_t::tt_int_value
		   && _tree->type != tree_t::tt_double_value
		   && _tree->type != tree_t::tt_char_value
		   && _tree->type != tree_t::tt_list
		   && _tree->type != tree_t::tt_opencontext
		   && _tree->type != tree_t::tt_closecontext;
}

bool AbstractParseTreeBase::isTree( Ident name ) const
{
	return _tree && _tree->type == name.val();
}

bool AbstractParseTreeBase::equalTree( Ident name ) const
{
	return _tree->type == name.val();
}

const char* AbstractParseTreeBase::type() const
{
	return _tree->type;
}

int AbstractParseTreeBase::nrParts() const
{
	list_t *parts = _tree != 0 ? _tree->c.parts : 0;
	int nr = 0;
	
	for (; parts; parts = parts->next)
		nr++;
		
	return nr;
}	

const char *AbstractParseTreeBase::filename() const
{
	return _tree->filename != 0 ? _tree->filename->value : 0;
}

int AbstractParseTreeBase::line() const 
{
	return _tree->line;
}

int AbstractParseTreeBase::column() const
{
	return _tree->column;
}

void AbstractParseTreeBase::print( FILE *f, bool compact ) const
{
	if (_tree == 0)
		fprintf(f, "[EMPTY]");
	else
		_tree->print(f, compact);

	if (!compact)
		fprintf(f, "\n");
};


// AbstractParseTree

AbstractParseTree::AbstractParseTree(const AbstractParseTree& lhs)
{
	SCONTEXT("__AbstractParseTree1")

	_tree = lhs._tree;
	if (_tree != 0)
	{
		_tree->refcount++;
		tree_t::alloced++;

		//printf("%8X +C ", _tree);
		//SContext::print(stdout);
		//printf("\n");
	}
}

AbstractParseTree::AbstractParseTree(tree_t *tree)
{
	SCONTEXT("__AbstractParseTree2")
	_tree = tree;
	if (_tree != 0)
	{
		_tree->refcount++;
		tree_t::alloced++;

		//printf("%8X +C ", _tree);
		//SContext::print(stdout);
		//printf("\n");
	}
}

AbstractParseTree& AbstractParseTree::operator=(const AbstractParseTree& lhs)
{
	SCONTEXT("=");
	tree_t::assign(_tree, lhs._tree);
	return *this;
}

AbstractParseTree::AbstractParseTree(const AbstractParseTree::iterator& lhs)
{
	SCONTEXT("__AbstractParseTree3");
	_tree = 0;
	tree_t::assign(_tree, lhs._list->first);
}

AbstractParseTree AbstractParseTree::makeList()
{
	AbstractParseTree result;
	result.createList();
	return result;
}

AbstractParseTree AbstractParseTree::makeTree(const Ident name)
{
	AbstractParseTree result;
	result.createTree(name);
	return result;
}

void AbstractParseTree::clear()
{
	if (_tree != 0)
		_tree->release();
	_tree = 0;
}
void AbstractParseTree::attach(AbstractParseTree& treeRef)
{
	SCONTEXT("attach");
	tree_t *old_tree = _tree;
	_tree = treeRef._tree;
	treeRef._tree = 0;
	if (old_tree != 0)
		old_tree->release();
}

void AbstractParseTree::release()
{
	SCONTEXT("release");
	if (_tree != 0)
		_tree->release();
	_tree = 0;
	if (_cursor != 0)
	{
		_cursor->detach();
		_cursor = 0;
	}
}


void AbstractParseTree::createIdent( const Ident ident )
{
	release();
	_tree = new tree_t(ident);
}

void AbstractParseTree::createStringAtom( const char *str )
{
	release();
	_tree = new tree_t(new string_t(str));
}

void AbstractParseTree::createStringAtom( String &str )
{
	if (_tree != 0 && _tree->type == tree_t::tt_str_value && str._str != 0 && _tree->c.str_value == str._str)
		return; // self assignment
	release();
	_tree = new tree_t(str._str);
}

void AbstractParseTree::createDoubleAtom( double value )
{
	release();
	_tree = new tree_t(value);
}

void AbstractParseTree::createCharAtom( const char value )
{
	release();
	_tree = new tree_t(value);
}

void AbstractParseTree::createIntAtom( long value )
{
	release();
	_tree = new tree_t(value);
}

void AbstractParseTree::createList( void )
{
	release();
	_tree = new tree_t(tree_t::tt_list);
}

void AbstractParseTree::createTree( const Ident name )
{
	release();
	assert(!name.empty());
	_tree = new tree_t(name.val());
}

void AbstractParseTree::setTreeName( const Ident name )
{
	assert(!name.empty());
	_tree->type = name.val();
}

void AbstractParseTree::insertChild( const AbstractParseTree& child )
{
	SCONTEXT("insertChild");

	assert(_cursor == 0);

	list_t *r_list;

	r_list = new list_t();
	tree_t::assign(r_list->first, child._tree);
	r_list->next = _tree->c.parts;
	_tree->c.parts = r_list;
} 

void AbstractParseTree::appendChild( const AbstractParseTree& child )
{
	SCONTEXT("appendChild");
	assert(_cursor == 0);

	list_t **r_list = &_tree->c.parts;

	while (*r_list != 0)
		r_list = &(*r_list)->next;

	*r_list = new list_t();
	tree_t::assign((*r_list)->first, child._tree);
}

void AbstractParseTree::dropLastChild()
{
	SCONTEXT("dropChild");
	assert(_cursor == 0);

	list_t **r_list = &_tree->c.parts;

	if (*r_list == 0)
		return;

	while ((*r_list)->next != 0)
		r_list = &(*r_list)->next;

	if ((*r_list)->first != 0)
		(*r_list)->first->release();
	delete (*r_list);
	*r_list = 0;
} 

void AbstractParseTree::createOpenContext()
{
	release();
	_tree = new tree_t(tree_t::tt_opencontext);
}

void AbstractParseTree::createCloseContext()
{
	release();
	_tree = new tree_t(tree_t::tt_closecontext);
}


AbstractParseTree AbstractParseTree::part( int i ) const
{
	list_t *parts = _tree->c.parts;

	for (; parts && i > 1; parts = parts->next, i--);

	AbstractParseTree result;
	if (parts != 0)
		tree_t::assign(result._tree, parts->first);
	
	return result;
}


void AbstractParseTree::setFileName(const String& filename)
{
	if (_tree != 0)
	{
		_tree->setFileName(filename._str);
	}
}

void AbstractParseTree::setLineColumn(int line, int column)
{
	if (_tree != 0)
	{
	/*
		if (_tree->line == line && _tree->column == column)
		{
			printf("WARN: position set to same value %d,%d for ", line, column);
			print(stdout, true);
			printf("\n");
		}
		else if (_tree->line != 0)
		{
			if (_tree->line == 3 && line == 3 && _tree->column == 8 && column == 6)
				printf("The problem: ");
			printf("ERROR: position reset from %d.%d to value %d,%d for ", _tree->line, _tree->column, line, column);
			print(stdout, true);
			printf("\n");
		}
		else
		{
			printf("position set to value %d,%d for ", line, column);
			print(stdout, true);
			printf("\n");
		}
	*/
		_tree->line = line;
		_tree->column = column;
	}
}


AbstractParseTree::iterator::iterator(const AbstractParseTree& tree)
{
	_list = tree._tree != 0 ? tree._tree->c.parts : 0;
}
AbstractParseTree::iterator::iterator(tree_t *tree)
{
	_list = tree != 0 ? tree->c.parts : 0;
}
bool AbstractParseTree::iterator::more()
{
	return _list != 0;
}
bool AbstractParseTree::iterator::isTree(Ident name)
{
	return _list != 0 && _list->first->type == name.val();
}
void AbstractParseTree::iterator::next()
{
	_list = _list->next;
}


void AbstractParseTree::Dispose()
{
	tree_t::Dispose();
	list_t::Dispose();
	tree_cursor_t::Dispose();
}


// AbstractParseTreeCursor

AbstractParseTreeCursor::AbstractParseTreeCursor()
  : _ref_prev(0), _next(0) {}

AbstractParseTreeCursor::AbstractParseTreeCursor(AbstractParseTree &root)
{
	if (root._cursor != 0)
		_cursor = root._cursor;
	else
	{
		// create the one cursor object with the AbstractParseTree
		_cursor = new tree_cursor_t(root, root._tree);
		_cursor->root = &root;
	}

	attach();
}

AbstractParseTreeCursor::AbstractParseTreeCursor(const AbstractParseTreeIteratorCursor& it)
{
	if (it.attached())
		_cursor = it._parent->child_for_part(it._part_nr, it._list->first);
	attach();
}

AbstractParseTreeCursor::AbstractParseTreeCursor(const AbstractParseTreeCursor& cursor)
{
	_cursor = cursor._cursor;
	attach();
}

AbstractParseTreeCursor::~AbstractParseTreeCursor()
{
	detach();
}

AbstractParseTreeCursor::operator AbstractParseTree() const
{
	return AbstractParseTree(_tree);
}

AbstractParseTreeCursor AbstractParseTreeCursor::part(int n)
{
	AbstractParseTreeCursor result;

	if (_cursor != 0)
	{
		if(!_cursor->tree->can_have_parts())
			assert(0);
		else
		{
			list_t *parts = _cursor->tree->c.parts;
			for (int i = 1; parts && i < n; i++)
				parts = parts->next;

			result._cursor = new tree_cursor_t;
			result._cursor->parent = _cursor;
			result._cursor->part_nr = n;
			result._cursor->tree = parts->first;

			result._cursor->next = _cursor->child_cursors;
			if (result._cursor->next != 0)
				result._cursor->next->ref_prev = &result._cursor->next;
			result._cursor->ref_prev = &_cursor->child_cursors;
			_cursor->child_cursors = result._cursor;

			result.attach();
		}
	}
	return result;
}

AbstractParseTreeCursor& AbstractParseTreeCursor::operator=(const AbstractParseTreeCursor& cursor)
{
	detach();
	_cursor = cursor._cursor;
	attach();
	return *this;
}

AbstractParseTreeCursor& AbstractParseTreeCursor::operator=(const AbstractParseTreeIteratorCursor& it)
{
	detach();
	if (it.attached())
		_cursor = it._parent->child_for_part(it._part_nr, it._list->first);
	attach();
	return *this;
}

bool AbstractParseTreeCursor::operator==(const AbstractParseTreeCursor& rhs) const
{
	if (!attached() || !rhs.attached())
		return !attached() && !rhs.attached();
	if (_tree == NULL || rhs._tree == NULL)
		return _tree == NULL && rhs._tree == NULL;
	if (type() != rhs.type())
		return false;
	if (isIdent())
		return identName() == rhs.identName();
	if (isString())
		return strcmp(string(), rhs.string()) == 0;
	if (isDouble())
		return doubleValue() == rhs.doubleValue();
	if (isChar())
		return charValue() == rhs.charValue();
	if (isTree() || isList())
	{
		AbstractParseTreeIteratorCursor lhsIt(*this);
		AbstractParseTreeIteratorCursor rhsIt(rhs);
		for (; lhsIt.more() && rhsIt.more(); lhsIt.next(), rhsIt.next())
			if (!(AbstractParseTreeCursor(lhsIt) == AbstractParseTreeCursor(rhsIt)))
				return false;
		return !lhsIt.more() && !rhsIt.more();
	}
	return false;
}

void AbstractParseTreeCursor::replaceBy(AbstractParseTree lhs)
{	// We need 'AbstractParseTree lhs' to prevent circular trees when a
	// parent node is assigned to one of its children.

	if (!attached())
	{
		assert(0); // assignment of new tree to detached cursor
		return;
	}

	_cursor->detach_children();

	if (_cursor->parent != 0)
		_cursor->parent->make_private_copy();

	if (lhs._tree != 0)
		lhs._tree->refcount++;

	_cursor->assign(lhs._tree);
}

void AbstractParseTreeCursor::appendChild(const AbstractParseTreeCursor& child)
{
	if (!attached() || !isList())
	{
		assert(0); // assignment of new tree to detached cursor
		return;
	}

	if (_cursor->parent != 0)
		_cursor->parent->make_private_copy();

	list_t **r_list = &_tree->c.parts;

	while (*r_list != 0)
		r_list = &(*r_list)->next;

	*r_list = new list_t();
	tree_t::assign((*r_list)->first, child._tree);
}


void AbstractParseTreeCursor::attach()
{
	if (_cursor == 0)
	{
		_tree = 0;
		_ref_prev = 0;
		_next = 0;
		return;
	}

	_tree = _cursor->tree;
	
	// Chain this with the cursors of the _cursor
	_next = _cursor->cursors;
	if (_next != 0)
		_next->_ref_prev = &_next;
	_ref_prev = &_cursor->cursors;
	_cursor->cursors = this;
}

void AbstractParseTreeCursor::detach()
{
	_tree = 0;
	if (_cursor == 0)
		return;

	*_ref_prev = _next;
	if (_next != 0)
		_next->_ref_prev = _ref_prev;
	_ref_prev = 0;
	_next = 0;

	if (_cursor != 0)
		_cursor->release();
	_cursor = 0;
}

// AbstractParseTreeIteratorCursor

AbstractParseTreeIteratorCursor::AbstractParseTreeIteratorCursor(const AbstractParseTreeCursor tree)
: AbstractParseTree::iterator(tree._cursor->tree), _parent(tree._cursor), _part_nr(1)
{
	_next = _parent->iterator_cursors;
	if (_next != 0)
		_next->_ref_prev = &_next;
	_ref_prev = &_parent->iterator_cursors;
	_parent->iterator_cursors = this;
}

AbstractParseTreeIteratorCursor::~AbstractParseTreeIteratorCursor()
{
	if (_parent != 0)
	{
		*_ref_prev = _next;
		if (_next != 0)
			_next->_ref_prev = _ref_prev;
		_parent->release();
	}
}

void AbstractParseTreeIteratorCursor::next()
{
	AbstractParseTree::iterator::next();
	_part_nr++;
}

void AbstractParseTreeIteratorCursor::erase()
{
	if (_parent == 0) { assert(0); return; }

	_parent->make_private_copy();

	// Fix iterator cursors:
	for (AbstractParseTreeIteratorCursor **ref_it = &_parent->iterator_cursors; *ref_it != 0;)
	{
		AbstractParseTreeIteratorCursor *iterator_cursor = *ref_it;

		if (iterator_cursor == this)
		{
			_list = _list->next;
			ref_it = &(*ref_it)->_next;
		}
		else if (iterator_cursor->_part_nr == _part_nr)
		{
			*iterator_cursor->_ref_prev = iterator_cursor->_next;
			iterator_cursor->_parent = 0;
			iterator_cursor->_ref_prev = 0;
			iterator_cursor->_next = 0;
			iterator_cursor->_list = 0;
		}
		else
		{
			if (iterator_cursor->_part_nr > _part_nr)
				iterator_cursor->_part_nr--;
			ref_it = &(*ref_it)->_next;
		}
	}

	// Fix child cursors:
	for (tree_cursor_t **ref_it = &_parent->child_cursors; *ref_it != 0;)
	{
		tree_cursor_t *child = *ref_it;
		if (child->part_nr == _part_nr)
		{
			*child->ref_prev = child->next;
			child->root = 0;
			child->parent = 0;
			child->ref_prev = 0;
			child->next = 0;
			child->tree = 0;
			child->detach();
		}
		else
		{
			if (child->part_nr > _part_nr)
				child->part_nr--;
			ref_it = &(*ref_it)->next;
		}
	}

	// remove my part from parent tree
	tree_t *tree = _parent->tree;
	list_t **ref_list = &tree->c.parts;
	for (int i = 1; i < _part_nr; i++)
		ref_list = &(*ref_list)->next;
	list_t *list = (*ref_list);
	*ref_list = _list;
	if (list->first != 0)
		list->first->release();
	delete list;
}

void AbstractParseTreeIteratorCursor::insert(AbstractParseTree child)
{
	if (_parent == 0) { assert(0); return; }

	_parent->make_private_copy();

	// Fix iterator cursors:
	for (AbstractParseTreeIteratorCursor *iterator_cursor = _parent->iterator_cursors; iterator_cursor != 0; iterator_cursor = iterator_cursor->_next)
	{
		if (iterator_cursor->_part_nr >= _part_nr && iterator_cursor != this)
			iterator_cursor->_part_nr++;
	}

	// Fix child cursors:
	for (tree_cursor_t *child_cursor = _parent->child_cursors; child_cursor != 0; child_cursor = child_cursor->next)
	{
		if (child_cursor->part_nr >= _part_nr)
			child_cursor->part_nr++;
	}

	// add new part to parent tree
	tree_t *tree = _parent->tree;
	list_t **ref_list = &tree->c.parts;
	for (int i = 1; i < _part_nr; i++)
		ref_list = &(*ref_list)->next;

	_list = new list_t();
	tree_t::assign(_list->first, child._tree);
	_list->next = *ref_list;
	*ref_list = _list;
}

/*
void AbstractParseTreeIteratorCursor::detach()
{
	if (!attached())
		return;

	// Detach from parent cursor
	_parent = 0;
	*_ref_prev = _next;
	_ref_prev = 0;
	_next = 0;

	// Detacht child cursors
	while (_iterators != 0)
		_iterators->detach();
}


tree_t *AbstractParseTreeIteratorCursor::clone()
{
	if (_parent != 0)
		_parent->clone();
	_list = _parent->_tree != 0 ? _parent->_tree->c.parts : 0;
	for (int i = 1; i < _pos; i++)
		_list = _list->next;

	return _list->first;
}
*/		

void AbstractParseTreeUnitTest()
{
	AbstractParseTree root;
	root.createList();
	{
		root.appendChild(1L);
		root.appendChild(2L);
		root.appendChild(3L);
	}

	for (int part_nr = 1; part_nr <= 3; part_nr++)
	{
		AbstractParseTree part = root.part(part_nr);
		assert(part.isInt());
		assert(part.intValue() == part_nr);
	}

	AbstractParseTree root2(root);

	for (int part_nr = 1; part_nr <= 3; part_nr++)
	{
		AbstractParseTree part = root2.part(part_nr);
		assert(part.isInt());
		assert(part.intValue() == part_nr);
	}

	AbstractParseTreeCursor root2_cursor(root2);

	for (int part_nr = 1; part_nr <= 3; part_nr++)
	{
		AbstractParseTree part = root2_cursor.part(part_nr);
		assert(part.isInt());
		assert(part.intValue() == part_nr);
	}

	AbstractParseTreeCursor part2 = root2_cursor.part(2);
	part2.replaceBy(5L);

	for (int part_nr = 1; part_nr <= 3; part_nr++)
	{
		AbstractParseTree part = root.part(part_nr);
		assert(part.isInt());
		assert(part.intValue() == part_nr);
	}

	for (int part_nr = 1; part_nr <= 3; part_nr++)
	{
		AbstractParseTree part = root2_cursor.part(part_nr);
		assert(part.isInt());
		assert(part.intValue() == (part_nr == 2 ? 5 : part_nr));
	}

	part2.replaceBy(root2);

	for (int part_nr = 1; part_nr <= 3; part_nr++)
	{
		AbstractParseTree part = root2_cursor.part(part_nr);
		if (part_nr != 2)
		{
			assert(part.isInt());
			assert(part.intValue() == part_nr);
		}
		else
		{
			assert(part.isList());
			for (int part_nr2 = 1; part_nr2 <= 3; part_nr2++)
			{
				AbstractParseTree part2 = part.part(part_nr2);
				assert(part2.isInt());
				assert(part2.intValue() == (part_nr2 == 2 ? 5 : part_nr2));
			}
		}
	}
}

#ifdef DEBUGALLOC
SContext::SContext(const char* name)
{
	_next = _top;
	_top = this;
	_name = name;
}
SContext::~SContext()
{
	_top = _next;
}

void SContext::print(FILE* f)
{
	if (_top)
		_top->_print(f);
}

void SContext::_print(FILE* f)
{
	if (_next)
		_next->_print(f);

	fprintf(f, "%s:", _name);
}

SContext* SContext::_top = 0;
#endif