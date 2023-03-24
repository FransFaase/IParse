#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "Ident.h"
#include "XMLParser.h"
#include "XMLIterator.h"

bool parse_xml(XMLIterator &it, AbstractParseTree& parent)
{
	if (!it.more())
		return false;

	if (it.isCloseTag())
		return false;
		
	if (it.isMeta())
	{
		AbstractParseTree child;
		child.createTree("?");
		child.setLineColumn(it.line, it.column);
		child.appendChild(it.value);
		parent.appendChild(child);
		it.next();
	}
	else if (it.isComment())
	{
		AbstractParseTree child;
		child.createTree("!");
		child.setLineColumn(it.line, it.column);
		child.appendChild(it.value);
		parent.appendChild(child);
		it.next();
	}
	else if (it.isOpenTag())
	{
		AbstractParseTree child;
		child.createTree(it.tag);
		child.setLineColumn(it.line, it.column);
		parent.appendChild(child);
		it.next();

		for (; it.isAttr(); it.next())
		{
			AbstractParseTree attr;
			attr.createTree("=");
			attr.setLineColumn(it.line, it.column);
			child.appendChild(attr);
			attr.appendChild(AbstractParseTree(Ident(it.attr)));
			attr.appendChild(AbstractParseTree(it.value));
		}
		while (parse_xml(it, child)) {}
		
		if (!it.isCloseTag())
			return false;
		
		it.next();
	}
	else if (it.isText())
	{
		parent.appendChild(AbstractParseTree(it.value.val()));
		it.next();
	}
	else
	{
		return false;
	}
	return true;
}

bool XMLParser::parse(const TextFileBuffer& textBuffer, AbstractParseTree& result)
{
	result.createTree("XML");
	XMLIterator it(textBuffer);
	parse_xml(it, result);
	
	return !it.more();
}

