#include <string.h>
#include "Ident.h"

struct hexa_hash_tree_t
{	bool has_children;
	union
	{	const char *string;
		hexa_hash_tree_t **children;
	} data;
};
typedef struct hexa_hash_tree_t *hexa_hash_tree_p;

const char* strcopy(const char *s)
{
	int len = strlen(s);
	char* result = new char[len+1];
	strcpy(result, s);
	return result;
}

const char* Ident::ident_unify(const char* id) const
/*	Returns a unique address representing the string. If
	the string does not occure in the store, it is added.
*/
{
	static hexa_hash_tree_p hash_tree = 0;
	hexa_hash_tree_p *r_node = &hash_tree;
	const char *vs = id;
	int depth;
	int mode = 0;
	
	if (*id == '\0')
		return _empty;

	for (depth = 0; ; depth++)
	{	hexa_hash_tree_p node = *r_node;

		if (node == 0)
		{	node = new hexa_hash_tree_t();
			node->has_children = false;
			node->data.string = strcopy(id);
			*r_node = node;
			return node->data.string;
		}

		if (!node->has_children)
		{	const char *cs = node->data.string;
			hexa_hash_tree_p *children;
			unsigned short i, v = 0;

			if (*cs == *id && strcmp(cs+1, id+1) == 0)
				return node->data.string;

			children = new hexa_hash_tree_p[16];
			for (i = 0; i < 16; i++)
				children[i] = 0;

			i = strlen(cs);
			if (depth <= i)
				v = ((unsigned char)cs[depth]) & 15;
			else if (depth <= i*2)
				v = ((unsigned char)cs[depth-i-1]) >> 4;

			children[v] = node;

			node = new hexa_hash_tree_t;
			node->has_children = true;
			node->data.children = children;
			*r_node = node;
		}
		{	unsigned short v;
			if (*vs == '\0')
			{	v = 0;
				if (mode == 0)
				{	mode = 1;
					vs = id;
				}
			}
			else if (mode == 0)
				v = ((unsigned short)*vs++) & 15;
			else
				v = ((unsigned short)*vs++) >> 4;

			r_node = &node->data.children[v];
		}
	}
}

char* Ident::_empty = "";
