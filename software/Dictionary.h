#ifndef _INCLUDED_DICTIONARY_H
#define _INCLUDED_DICTIONARY_H

#include "Ident.h"
#include "AbstractParseTree.h"

class Dictionary
{
public:
	Dictionary();
	~Dictionary();
	void clear();
	void setLanguages(const String& from_lang, const String& to_lang);
	void add(const String& from_text, const String& to_text);
	void add(const Ident& section, const Ident& item, const String& from_text, const String& to_text);
	void establishMajorVariants();
	void printMultiple(FILE *fout);

	const String& fromLang() const { return _from_lang; }
	const String& toLang() const { return _to_lang; }

	bool translate(const String& from_text, String &to_text);
	bool translate(const String& from_text, Ident section, Ident item, String &to_text);

	struct Entry;
	struct Variant;
	struct Example;

	class VariantIterator;
	class EntryIterator
	{
		friend class VariantIterator;
	public:
		EntryIterator(const Dictionary& dictionary);
		bool more();
		void next();
		const String fromText();
		bool hasMajorVariant();
		const String& majorToText();
	private:
		Entry* _entry;
	};

	class ExampleIterator;
	class VariantIterator
	{
		friend class ExampleIterator;
	public:
		VariantIterator(const EntryIterator& entryIterator);
		bool more();
		void next();
		const String& toText();
	private:
		Variant* _major_variant;
		Variant* _variant;
	};

	class ExampleIterator
	{
	public:
		ExampleIterator(const VariantIterator& variantIterator);
		bool more();
		void next();
		const Ident section();
		const Ident item();
	private:
		Example* _example;
	};

private:
	String _from_lang;
	String _to_lang;
	Entry* _entries;
	Example* _examples;
	Example** _ref_example;
};


#endif // _INCLUDED_DICTIONARY_H
