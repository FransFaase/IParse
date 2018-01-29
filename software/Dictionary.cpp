#include <string.h>
#include <stdio.h>
#include "Ident.h"

#include "Dictionary.h"

struct Dictionary::Entry
{
	Entry(const String& n_from_text, Entry* n_next) : from_text(n_from_text), major_variant(0), variants(0), next(n_next) {}
	String from_text;
	Dictionary::Variant *major_variant;
	Dictionary::Variant *variants;
	Entry* next;
};

struct Dictionary::Variant
{
	Variant(const String& n_to_text, Variant* n_next) : to_text(n_to_text), nr_examples(0), examples(0), next(n_next) {}
	//DictEntry(DictEntry *n_next) : entry(0), next(n_next) {}
	String to_text;
	int nr_examples;
	Dictionary::Example* examples;
	Variant* next;
};

struct Dictionary::Example
{
	Example(Ident n_section, Ident n_item) : section(n_section), item(n_item), next(0), next_of_variant(0) {}
	Ident section;
	Ident item;
	Example* next;
	Example* next_of_variant;
};

Dictionary::Dictionary()
 : _entries(0), _examples(0)
{
	_ref_example = &_examples;
}

Dictionary::~Dictionary()
{
	clear();
}

void Dictionary::clear()
{
	for (Entry* entry = _entries; entry != 0;)
	{
		for (Variant* variant = entry->variants; variant != 0;)
		{
			for (Example* example = variant->examples; example != 0;)
			{
				Example* next_example = example->next_of_variant;
				delete example;
				example = next_example;
			}
			Variant* next_variant = variant->next;
			delete variant;
			variant = next_variant;
		}
		Entry* next_entry = entry->next;
		delete entry;
		entry = next_entry;
	}
	_entries = 0;
	_ref_example = &_examples;
	_examples = 0;
}

void Dictionary::setLanguages(const String& from_lang, const String& to_lang)
{
	_from_lang = from_lang;
	_to_lang = to_lang;
}

void Dictionary::add(const String& from_text, const String& to_text)
{
	Entry** ref_entry = &_entries;
	while (*ref_entry != 0 && (*ref_entry)->from_text < from_text)
		ref_entry = &(*ref_entry)->next;
	if (*ref_entry == 0 || (*ref_entry)->from_text > from_text)
		(*ref_entry) = new Entry(from_text, (*ref_entry));

	Variant** ref_variant = &(*ref_entry)->variants;
	while (*ref_variant != 0 && (*ref_variant)->to_text < to_text)
		ref_variant = &(*ref_variant)->next;
	if (*ref_variant != 0 && (*ref_variant)->to_text == to_text)
		return;

	(*ref_variant) = new Variant(to_text, *ref_variant);
	if ((*ref_entry)->major_variant == 0)
		(*ref_entry)->major_variant = (*ref_variant);
}

void Dictionary::add(const Ident& section, const Ident& item, const String& from_text, const String& to_text)
{
	Entry** ref_entry = &_entries;
	while (*ref_entry != 0 && (*ref_entry)->from_text < from_text)
		ref_entry = &(*ref_entry)->next;
	if (*ref_entry == 0 || (*ref_entry)->from_text > from_text)
		(*ref_entry) = new Entry(from_text, (*ref_entry));

	Variant** ref_variant = &(*ref_entry)->variants;
	while (*ref_variant != 0 && (*ref_variant)->to_text < to_text)
		ref_variant = &(*ref_variant)->next;
	if (*ref_variant == 0 || (*ref_variant)->to_text > to_text)
		(*ref_variant) = new Variant(to_text, *ref_variant);

	(*ref_variant)->nr_examples++;
	Example** ref_example = &(*ref_variant)->examples;
	while (*ref_example != 0)
		ref_example = &(*ref_example)->next_of_variant;
	(*ref_example) = new Example(section, item);
	(*_ref_example) = (*ref_example);
	_ref_example = &(*_ref_example)->next;
}

void Dictionary::establishMajorVariants()
{
	for (Entry* entry = _entries; entry != 0; entry = entry->next)
	{
		int nr = 0;
		int sum = 0;
		Variant* max_variant = 0;
		for (Variant* variant = entry->variants; variant != 0; variant = variant->next)
		{
			nr++;
			sum += variant->nr_examples;
			if (max_variant == 0 || variant->nr_examples > max_variant->nr_examples)
				max_variant = variant;
		}
		if (nr == 1 || max_variant->nr_examples > sum / 2)
			entry->major_variant = max_variant;
	}
}

void Dictionary::printMultiple(FILE *fout)
{
	for (EntryIterator entryIterator(*this); entryIterator.more(); entryIterator.next())
	{
		if (entryIterator.hasVariants())
		{
			fprintf(fout, "|%s|\n", entryIterator.fromText().val());
			for (VariantIterator variantIterator(entryIterator, /*include major=*/true); variantIterator.more(); variantIterator.next())
			{
				fprintf(fout, "=> |%s| at ", variantIterator.toText().val());
				bool first = true;
				for (Dictionary::ExampleIterator exampleIterator(variantIterator); exampleIterator.more(); exampleIterator.next())
				{
					if (first)
						first = false;
					else
						fprintf(fout, ", ");
					fprintf(fout, "%s.%s", exampleIterator.section().val(), exampleIterator.item().val());
				}
				fprintf(fout, "\n");
			}
		}
	}
}

Dictionary::EntryIterator::EntryIterator(const Dictionary& dictionary)
{
	_entry = dictionary._entries;
}

bool Dictionary::EntryIterator::more()
{
	return _entry != 0;
}

void Dictionary::EntryIterator::next()
{
	_entry = _entry->next;
}

const String Dictionary::EntryIterator::fromText()
{
	return _entry->from_text;
}

bool Dictionary::EntryIterator::hasVariants()
{
	return _entry->variants != 0 && _entry->variants->next != 0;
}

bool Dictionary::EntryIterator::hasMajorVariant()
{
	return _entry->major_variant != 0;
}

const String& Dictionary::EntryIterator::majorToText()
{
	return _entry->major_variant->to_text;
}

Dictionary::VariantIterator::VariantIterator(const EntryIterator& entryIterator, bool include_major)
{
	_major_variant = include_major ? 0 : entryIterator._entry->major_variant;
	_variant = entryIterator._entry->variants;
	if (_variant != 0 && _variant == _major_variant)
		_variant = _variant->next;
}

bool Dictionary::VariantIterator::more()
{
	return _variant != 0;
}

void Dictionary::VariantIterator::next()
{
	_variant = _variant->next;
	if (_variant != 0 && _variant == _major_variant)
		_variant = _variant->next;
}

const String& Dictionary::VariantIterator::toText()
{
	return _variant->to_text;
}

Dictionary::ExampleIterator::ExampleIterator(const VariantIterator& variantIterator)
{
	_example = variantIterator._variant->examples;
}

bool Dictionary::ExampleIterator::more()
{
	return _example != 0;
}

void Dictionary::ExampleIterator::next()
{
	_example = _example->next_of_variant;
}

const Ident Dictionary::ExampleIterator::section()
{
	return _example->section;
}

const Ident Dictionary::ExampleIterator::item()
{
	return _example->item;
}

bool Dictionary::translate(const String& from_text, String &to_text) const
{
	for (Dictionary::EntryIterator entryIterator(*this); entryIterator.more(); entryIterator.next())
	{
		if (entryIterator.fromText() == from_text)
		{
			if (entryIterator.hasMajorVariant())
			{
				to_text = entryIterator.majorToText();
				return true;
			}
		}
	}
	fprintf(stderr, "No translation for |%s|\n", from_text.val());
	return false;
}

bool Dictionary::translate(const String& from_text, Ident section, Ident item, String &to_text) const
{
	for (Dictionary::EntryIterator entryIterator(*this); entryIterator.more(); entryIterator.next())
	{
		if (entryIterator.fromText() == from_text)
		{
			for (Dictionary::VariantIterator variantIterator(entryIterator); variantIterator.more(); variantIterator.next())
			{
				for (Dictionary::ExampleIterator exampleIterator(variantIterator); exampleIterator.more(); exampleIterator.next())
				{
					if (exampleIterator.section() == section && exampleIterator.item() == item)
					{
						to_text = variantIterator.toText();
						return true;
					}
				}
			}

			if (entryIterator.hasMajorVariant())
			{
				to_text = entryIterator.majorToText();
				return true;
			}
		}
	}
	fprintf(stderr, "No translation for |%s| for %s %s\n", from_text.val(), section.val(), item.val());
	return false;
}

bool
Dictionary::translate(const String& from_text, Ident item, String &to_text) const
{
	return translate(from_text, Ident(), item, to_text);
}

bool
Dictionary::translate(const String& from_text, Ident section, int nr, String &to_text) const
{
	return translate(from_text, to_text);
}


/*
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
	TextFileBuffer textBuffer;
	PlainFileReader plainFileReader;
	plainFileReader.read(fin, textBuffer);
	AbstractParseTree tree;
	XMLParser xmlParser;
	if (!xmlParser.parse(textBuffer, tree))
		return;

	FILE *f = fopen("out.txt", "wt");
	tree.print(f, false);
	fclose(f);

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
										setLanguages(from_lang, to_lang);
										first = false;
									}
									if (from_lang == _from_lang && to_lang == _to_lang)
										add(tuid, from_text, to_text);
								}
							}
						}
					}
				}
			}
		}
	}
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

	XMLStreamer streamer(fout, /* compact * /false);

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
