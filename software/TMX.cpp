#include <string.h>
#include <stdio.h>
#include "Ident.h"
#include "String.h"

#include "TMX.h"

#include "XMLStreamer.h"

#include "TextFileBuffer.h"
#include "TextReader.h"
#include "XMLParser.h"

void TMX::loadFromOmegaT(FILE *fin, Dictionary &dictionary)
{
	dictionary.clear();

	TextFileBuffer textBuffer;
	PlainFileReader plainFileReader;
	plainFileReader.read(fin, textBuffer);
	AbstractParseTree tree;
	XMLParser xmlParser;
	if (!xmlParser.parse(textBuffer, tree))
		return;

	/*
	FILE *f = fopen("out.txt", "wt");
	tree.print(f, false);
	fclose(f);
	*/

	if (tree.isTree("XML"))
	{
		static Ident id_quest("?");
		static Ident id_exlam("!");
		static Ident id_tmx("tmx");
		static Ident id_is("=");
		static Ident id_header("header");
		static Ident id_body("body");
		static Ident id_tu("tu");
		static Ident id_tuid("tuid");
		static Ident id_prop("prop");
		static Ident id_attr_type("type");
		static Ident id_tuv("tuv");
		static Ident id_lang("lang");
		static Ident id_seg("seg");

		AbstractParseTree::iterator rootIt(tree);
		while (   rootIt.more() 
			   && (   AbstractParseTree(rootIt).isTree(id_quest) 
				   || AbstractParseTree(rootIt).isTree(id_exlam)))
			rootIt.next();
		if (AbstractParseTree(rootIt).isTree(id_tmx))
		{
			AbstractParseTree rootTree(rootIt);
			AbstractParseTree::iterator xmlIt(rootTree);
			while (xmlIt.isTree(id_is))
				xmlIt.next();
			if (xmlIt.isTree(id_header))
				xmlIt.next();
			if (xmlIt.isTree(id_body))
			{
				AbstractParseTree bodyTree(xmlIt);
				bool first = true;
				for (AbstractParseTree::iterator bodyIt(bodyTree); bodyIt.more(); bodyIt.next())
				{
					AbstractParseTree part(bodyIt);
					if (part.isTree(id_tu))
					{
						AbstractParseTree::iterator tuIt(part);
						Ident tuid;
						for (; tuIt.isTree(id_is); tuIt.next())
						{
							AbstractParseTree attr(tuIt);
							if (attr.part(1).isIdent() && attr.part(1).identName() == id_tuid && attr.part(2).isString())
								tuid = attr.part(2).string().val();
						}
						Ident section;
						Ident item;
						for (; tuIt.isTree(id_prop); tuIt.next())
						{
							AbstractParseTree prop(tuIt);
							AbstractParseTree::iterator propIt(prop);

							bool is_id_prop = false;
							for (; propIt.isTree(id_is); propIt.next())
							{
								AbstractParseTree attr(propIt);
								if (   attr.part(1).isIdent() && attr.part(1).identName() == id_attr_type
									&& attr.part(2).isString() && attr.part(2).string() == "id")
									is_id_prop = true;
							}
							if (is_id_prop)
							{
								AbstractParseTree content(propIt);
								if (content.isString())
								{
									String string = content.stringValue();
									const char *str = string;
									const char *sep = str;
									for (const char* s = str; *s != '\0'; s++)
										if (*s == '/')
											sep = s;
									section = String(str, sep);
									item = String(sep+1);
								}
							}
						}
						String from_lang;
						String from_text;
						if (tuIt.isTree(id_tuv))
						{
							AbstractParseTree tuTree(tuIt);
							AbstractParseTree::iterator tuvIt(tuTree);
							for (; tuvIt.isTree(id_is); tuvIt.next())
							{
								AbstractParseTree attr(tuvIt);
								if (attr.part(1).isIdent() && attr.part(1).identName() == id_lang && attr.part(2).isString())
									from_lang = attr.part(2).string();
							}
							if (tuvIt.isTree(id_seg))
							{
								AbstractParseTree segTree(tuvIt);
								if (segTree.part(1).isString())
									from_text = segTree.part(1).string();
							}
							tuIt.next();
						}
						if (tuIt.isTree(id_tuv))
						{
							String to_lang;
							AbstractParseTree tuTree(tuIt);
							AbstractParseTree::iterator tuvIt(tuTree);
							for (; tuvIt.isTree(id_is); tuvIt.next())
							{
								AbstractParseTree attr(tuvIt);
								if (attr.part(1).isIdent() && attr.part(1).identName() == id_lang && attr.part(2).isString())
									to_lang = attr.part(2).string();
							}
							for (; tuvIt.isTree(id_seg); tuvIt.next())
							{
								String to_text;
								AbstractParseTree segTree(tuvIt);
								if (segTree.part(1).isString())
									to_text = segTree.part(1).string();
								
								if (!from_lang.empty() && !to_lang.empty())
								{
									if (first)
									{
										dictionary.setLanguages(from_lang, to_lang);
										first = false;
									}
									if (from_lang == dictionary.fromLang() && to_lang == dictionary.toLang())
									{
										if (!item.empty())
											dictionary.add(section, item, from_text, to_text);
										else
											dictionary.add(from_text, to_text);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void TMX::saveToOmegaT(FILE *fout, const Dictionary &dictionary, const char* source_file_name)
{
	XMLStreamer streamer(fout, /*compact*/false, "  ");

	for (const char *s = source_file_name; *s != '\0'; s++)
		if (*s == '/' || *s == '\\')
			source_file_name = s + 1;

	streamer.addHeader("xml version=\"1.0\" encoding=\"UTF-8\"");
	streamer.addMeta("DOCTYPE tmx SYSTEM \"tmx11.dtd\"");
	streamer.openTag("tmx");
	streamer.addAttribute("version", "1.1");
	streamer.openTag("header");
	streamer.addAttribute("creationtool", "RcTransl");
	streamer.addAttribute("o-tmf", "OmegaT TMX");
	streamer.addAttribute("adminlang", "EN-US");
	streamer.addAttribute("datatype", "plaintext");
	streamer.addAttribute("creationtoolversion", "0.1");
	streamer.addAttribute("segtype", "paragraph");
	streamer.addAttribute("srclang", dictionary.fromLang().val());
	streamer.closeTag();
	streamer.openTag("body");
	streamer.addMeta("-- Default translations --", 0);

	for (Dictionary::EntryIterator entryIterator(dictionary); entryIterator.more(); entryIterator.next())
	{
		if (entryIterator.hasMajorVariant())
		{
			streamer.openTag("tu");
			streamer.openTag("tuv");
			streamer.addAttribute("lang", dictionary.fromLang().val());
			streamer.openTag("seg");
			streamer.addContent(entryIterator.fromText());
			streamer.closeTag();
			streamer.closeTag();
			streamer.openTag("tuv");
			streamer.addAttribute("lang", dictionary.toLang().val());
			streamer.openTag("seg");
			streamer.addContent(entryIterator.majorToText());
			streamer.closeTag();
			streamer.closeTag();
			streamer.closeTag();
		}
	}

	streamer.addMeta("-- Alternative translations --", 0);

	for (Dictionary::EntryIterator entryIterator(dictionary); entryIterator.more(); entryIterator.next())
	{
		for (Dictionary::VariantIterator variantIterator(entryIterator); variantIterator.more(); variantIterator.next())
		{
			for (Dictionary::ExampleIterator exampleIterator(variantIterator); exampleIterator.more(); exampleIterator.next())
			{
				streamer.openTag("tu");
				streamer.openTag("prop");
				streamer.addAttribute("type", "file");
				streamer.addContent(source_file_name);
				streamer.closeTag();
				streamer.openTag("prop");
				streamer.addAttribute("type", "id");
				char combined_id[400];
				sprintf(combined_id, "%s/%s", exampleIterator.section().val(), exampleIterator.item().val());
				streamer.addContent(combined_id);
				streamer.closeTag();
				streamer.openTag("tuv");
				streamer.addAttribute("lang", dictionary.fromLang().val());
				streamer.openTag("seg");
				streamer.addContent(entryIterator.fromText());
				streamer.closeTag();
				streamer.closeTag();
				streamer.openTag("tuv");
				streamer.addAttribute("lang", dictionary.toLang().val());
				streamer.openTag("seg");
				streamer.addContent(variantIterator.toText());
				streamer.closeTag();
				streamer.closeTag();
				streamer.closeTag();
			}
		}
	}

	streamer.closeTag();
	streamer.closeTag();
}


/*
struct TMX::Entry
{
	Entry(const Ident& n_ident, const String& n_from_text, const String& n_to_text)
		: ident(n_ident), from_text(n_from_text), to_text(n_to_text), repeated(false), next_dict_entry(0), next(0) {}
	Ident ident;
	String from_text;
	String to_text;
	bool repeated;
	Entry* next_dict_entry;
	Entry* next;
};

struct TMX::DictEntry
{
	DictEntry(DictEntry *n_next) : entry(0), next(n_next) {}
	TMX::Entry* entry;
	DictEntry* next;
};

TMX::TMX()
 : _entries(0), _dict_entries(0)
{
	_ref_entries = &_entries;
}

void TMX::clear()
{
	for (Entry* entry = _entries; entry != 0;)
	{
		Entry* next = entry->next;
		delete entry;
		entry = next;
	}
	_entries = 0;
	for (DictEntry* dictEntry = _dict_entries; dictEntry != 0;)
	{
		DictEntry* next = dictEntry->next;
		delete dictEntry;
		dictEntry = next;
	}
	_dict_entries = 0;
}

void TMX::setLanguages(const String& from_lang, const String& to_lang)
{
	_from_lang = from_lang;
	_to_lang = to_lang;
}

void TMX::add(const Ident& ident, const String& from_text, const String& to_text)
{
	TMX::Entry* entry = new TMX::Entry(ident, from_text, to_text);
	*_ref_entries = entry;
	_ref_entries = &entry->next;

	TMX::DictEntry** ref_dict_entry = &_dict_entries;
	while ((*ref_dict_entry) != 0 && (*ref_dict_entry)->entry->from_text < entry->from_text)
		ref_dict_entry = &(*ref_dict_entry)->next;

	if ((*ref_dict_entry) == 0 || (*ref_dict_entry)->entry->from_text > entry->from_text)
		*ref_dict_entry = new DictEntry(*ref_dict_entry);

	TMX::Entry** ref_entry = &(*ref_dict_entry)->entry;
	for (; *ref_entry != 0; ref_entry = &(*ref_entry)->next_dict_entry)
		if ((*ref_entry)->to_text == entry->to_text)
			entry->repeated = true;

	*ref_entry = entry;
}

void TMX::load(FILE *fin)
{
	}

void TMX::printMultiple(FILE *fout)
{
	for (DictEntry* dict_entry = _dict_entries; dict_entry != 0; dict_entry = dict_entry->next)
	{
		bool has_multiples = false;
		for (Entry* entry = dict_entry->entry->next_dict_entry; entry != 0; entry = entry->next_dict_entry)
			if (!entry->repeated)
				has_multiples = true;

		if (has_multiples)
		{
			fprintf(fout, "|%s|\n", dict_entry->entry->from_text.val());
			for (Entry* entry = dict_entry->entry; entry != 0; entry = entry->next_dict_entry)
				fprintf(fout, "%s |%s|\n", entry->ident.val(), entry->to_text.val());
			fprintf(fout, "\n");
		}
	}
}

void TMX::saveAs(FILE* fout, bool include_idents, bool alphabetic)
{
	if (_from_lang.empty() || _to_lang.empty())
		return;

	XMLStreamer streamer(fout, /*compact* /false);

	streamer.addHeader("xml version=\"1.0\" encoding=\"UTF-8\"");
	streamer.addMeta("DOCTYPE tmx SYSTEM \"tmx11.dtd\"");
	streamer.openTag("tmx");
	streamer.addAttribute("version", "1.1");
	streamer.openTag("header");
	streamer.addAttribute("creationtool", "RcTransl");
	streamer.addAttribute("o-tmf", "OmegaT TMX");
	streamer.addAttribute("adminlang", "EN-US");
	streamer.addAttribute("datatype", "plaintext");
	streamer.addAttribute("creationtoolversion", "0.1");
	streamer.addAttribute("segtype", "paragraph");
	streamer.addAttribute("srclang", _from_lang);
	streamer.closeTag();
	streamer.openTag("body");
	streamer.addMeta("-- Default translations --", 0);

	if (alphabetic)
	{
		for (DictEntry* dict_entry = _dict_entries; dict_entry != 0; dict_entry = dict_entry->next)
		{
			if (!include_idents)
			{
				streamer.openTag("tu");
				streamer.openTag("tuv");
				streamer.addAttribute("lang", _from_lang);
				streamer.openTag("seg");
				streamer.addContent(dict_entry->entry->from_text);
				streamer.closeTag();
				streamer.closeTag();
				streamer.openTag("tuv");
				streamer.addAttribute("lang", _to_lang);
				for (Entry* entry = dict_entry->entry; entry != 0; entry = entry->next_dict_entry)
					if (!entry->repeated)
					{
						streamer.openTag("seg");
						streamer.addContent(entry->to_text);
						streamer.closeTag();
					}
				streamer.closeTag();
				streamer.closeTag();
			}
			else
			{
				for (Entry* entry = dict_entry->entry; entry != 0; entry = entry->next_dict_entry)
				{
					streamer.openTag("tu");
					if (!entry->ident.empty())
						streamer.addAttribute("tuid", entry->ident.val());
					streamer.openTag("tuv");
					streamer.addAttribute("lang", _from_lang);
					streamer.openTag("seg");
					streamer.addContent(dict_entry->entry->from_text);
					streamer.closeTag();
					streamer.closeTag();
					streamer.openTag("tuv");
					streamer.addAttribute("lang", _to_lang);
					streamer.openTag("seg");
					streamer.addContent(entry->to_text);
					streamer.closeTag();
					streamer.closeTag();
					streamer.closeTag();
				}
			}
		}
	}
	else
	{
		for (Entry* entry = _entries; entry != 0; entry = entry->next)
			if (include_idents || !entry->repeated)
			{
				streamer.openTag("tu");
				if (include_idents && !entry->ident.empty())
					streamer.addAttribute("tuid", entry->ident.val());
				streamer.openTag("tuv");
				streamer.addAttribute("lang", _from_lang);
				streamer.openTag("seg");
				streamer.addContent(entry->from_text);
				streamer.closeTag();
				streamer.closeTag();
				streamer.openTag("tuv");
				streamer.addAttribute("lang", _to_lang);
				streamer.openTag("seg");
				streamer.addContent(entry->to_text);
				streamer.closeTag();
				streamer.closeTag();
				streamer.closeTag();
			}
	}

	streamer.addMeta("-- Alternative translations --", 0);
	streamer.closeTag();
	streamer.closeTag();
}
*/
