/*	SharedStack: a stack template -- Copyright (C) 2003 Frans Faase

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

GNU General Public License:
	http://home.planet.nl/~faase009/GNU.txt

Description:
	the SharedStack template class implements a stack
	template class, with the special property that making
	copies of a stack is very cheap and that it is memory
	efficient.
	
	To execute unit tests, compile as single file with -DUNITTEST
	switch.
	
Origin:
	http://www.iwriteiam.nl/SharedStack_h.txt
Author and email:
	http://www.iwriteiam.nl/
*/

#if !defined(SHARED_STACK__INCLUDED_)
#define SHARED_STACK__INCLUDED_

template <class T>
class SharedStack
{
private:
	class element
	{
	public:
		T _v;
		element* _parent;
		long _ref_count;
		element(const T& v, element* parent)
		{	_v = v;
			_parent = parent;
			_ref_count = 1;
			//printf("\nCreated element %lx of '%c' and parent %lx", (long)this, v, (long)parent);
		}
		static void assign(element*& lhs, const element* rhs)
		{
			element* old = (element*)lhs;
			lhs = (element*)rhs;
			if (lhs)
				lhs->_ref_count++;
			if (old)
				old->release();
		}
		static bool equal(const element* lhs, const element* rhs)
		{	return    (lhs == rhs)
				   || (   lhs != 0 && rhs != 0
				   	   && lhs->_v == rhs->_v 
				   	   && equal(lhs->_parent, rhs->_parent));
		}
		inline static void push(element*& elem, const T& v)
		{	elem = new element(v, elem);
		}
		static void pop(element*& elem)
		{	element* old = elem;
			elem = elem->_parent;
			if (elem)
				lhs->_ref_count++;
			if (--old->_ref_count == 0)
				delete old;
		}
		static void set(element*& elem, const T& v)
		{
			if (elem->_v == v)
				return;
			if (elem->_ref_count == 1)
				elem->_v = v;
			else
			{	// pop
				elem->_ref_count--;
				element* parent = elem->_parent;
				if (parent)
					parent->_ref_count++;
				// push
				elem = new element(v, parent);
			}
		}
		void release()
		{	if (--_ref_count == 0)
			{	if (_parent != 0)
					_parent->release();
				//printf("\nDelete element %lx with '%c'", (long)this, _v);
				delete this;
			}
		}
	};
	element *_me;
	
public:
	
	SharedStack()
	{	_me = 0;
	}
	SharedStack(SharedStack& other)
	{	element::assign(_me, other._me);
	}
	SharedStack& operator=(const SharedStack& rhs)
	{	element::assign(_me, rhs._me);
		return *this;
	}
	inline bool operator==(const SharedStack& rhs)
	{	return element::equal(_me, rhs._me);
	}
	inline bool operator!=(const SharedStack& rhs)
	{	return !element::equal(_me, rhs._me);
	}
	~SharedStack()
	{	empty();
	}
	
	
	inline void empty()
	{	if (!isEmpty())
		{	_me->release();
			_me = 0;
		}
	}
	inline bool isEmpty()
	{	return _me == 0;
	}
	//inline void push(T& v)
	//{	element::push(_me, v);
	//}
	inline void push(const T v)
	{	element::push(_me, v);
	}
	inline void pop()
	{	if (!isEmpty())
			element::assign(_me, _me->_parent);
	}
	void set(T& v)
	{	if (!isEmpty())
			element::set(_me, v);
	}
	void set(const T v)
	{	if (!isEmpty())
			element::set(_me, v);
	}
	inline topIs(T& v)
	{	return !isEmpty() && _me->_v == v;
	}
	inline bool topIs(const T v)
	{	return !isEmpty() && _me->_v == v;
	}
	inline T& top()           // Produces memory violation 
	{	return _me->_v;    // when not empty
	}
	inline const T* operator[](int n)
	{	const element *it = _me;
		while (it != 0 && n > 0)
		{	it = it->_parent;
			n--;
		}
		return it != 0 ? &it->_v : 0;
	}
#ifdef UNITTEST
	inline const long count(int n)
	{	const element *it = _me;
		while (it != 0 && n > 0)
		{	it = it->_parent;
			n--;
		}
		return it != 0 ? it->_ref_count : -1;
	}
#endif
};


#ifdef UNITTEST

#include <stdio.h>

void test(char *name, char *state, SharedStack<char>& stack)
{
	bool passed = false;
	char *s = state;
	for (int i = 0; ; i++, s += 2)
	{	const char *v = stack[i];
		if (v == 0)
		{	passed = (s[0] == '\0');
			break;
		}
		else if (s[0] != *v || (s[1] - '0') != stack.count(i))
			break;
	}
	if (passed)
		return;
		
	printf("\nTest %s: %s; stack bevat: |", 
		   name, passed ? "Passed" : "Failed");
	const char *v; 
	for (int i = 0; (v = stack[i]) != 0; i++)
		printf("%c%ld", *v, stack.count(i));
	printf("|, verwachte |%s|", state);
}


main()
{
	SharedStack<char> stack;
	test("1", "", stack);
	
	stack.push('i');
	test("2", "i1", stack);
	
	stack.set('a');
	test("2A", "a1", stack);
	
	stack.push('b');
	test("3", "b1a1", stack);
	
	stack.push('c');
	test("4", "c1b1a1", stack);
	
	stack.pop();
	test("5", "b1a1", stack);
	
	SharedStack<char> stack2(stack);
	test("6a", "b2a1", stack);
	test("6b", "b2a1", stack2);

	if (stack != stack2)
		printf("\n Test equality1: Failed");
			
	stack2.push('d');
	test("7a", "b2a1", stack);
	test("7b", "d1b2a1", stack2);
	
	if (stack == stack2)
		printf("\n Test equality2: Failed");

	stack2.pop();
	test("8a", "b2a1", stack);
	test("8b", "b2a1", stack2);
	
	if (stack != stack2)
		printf("\n Test equality3: Failed");

	stack2.pop();
	test("8a", "b1a2", stack);
	test("8b", "a2", stack2);
	
	SharedStack<char> stack3;
	stack3 = stack;
	test("9a", "b2a2", stack);
	test("9b", "a2", stack2);
	test("9c", "b2a2", stack3);
	
	stack3.pop();
	test("10a", "b1a3", stack);
	test("10b", "a3", stack2);
	test("10c", "a3", stack3);
	
	stack3.push('e');
	test("11a", "b1a3", stack);
	test("11b", "a3", stack2);
	test("11c", "e1a3", stack3);
	
	stack3.push('f');
	test("12a", "b1a3", stack);
	test("12b", "a3", stack2);
	test("12c", "f1e1a3", stack3);
	
	stack.empty();
	test("13a", "", stack);
	test("13b", "a2", stack2);
	test("13c", "f1e1a2", stack3);
	
	stack2.set('g');
	test("14a", "", stack);
	test("14b", "g1", stack2);
	test("14c", "f1e1a1", stack3);
	
	stack2.empty();
	test("15a", "", stack);
	test("15b", "", stack2);
	test("15c", "f1e1a1", stack3);
	
	stack3.empty();
	test("16a", "", stack);
	test("16b", "", stack2);
	test("16c", "", stack3);
	
	stack3.push('h');
	test("17", "h1", stack3);
	
	stack3.push('i');
	test("18", "i1h1", stack3);
	
	stack = stack3;
	test("19a", "i2h1", stack);
	test("19b", "i2h1", stack3);
	
	stack3.set('j');
	test("20a", "i1h2", stack);
	test("20b", "j1h2", stack3);
	
	stack3.set('i');
	test("20a", "i1h2", stack);
	test("20b", "i1h2", stack3);

	if (stack != stack3)
		printf("\n Test equality4: Failed");
	
}

#endif

#endif // SHARED_STACK__INCLUDED_