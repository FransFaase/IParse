IParse
------
IParse is an interpreting parser, meaning that it reads
a grammar and interpret this to parse another file. It
also uses a parsing driven scanner approach, where the
parser calls the scanner to see if a certain type of
scanner symbol is found on the input. A number of scanners
are provided, including a raw scanner, which gives access
to the raw input. The grammar allows definition of character
ranges and white space terminals, thus allowing a scanner
to be specified in the grammar.

Several parsing algorithms are provided and can be selected
from the command line. The default parsing algorithm is a
back-tracking parser, which uses memorization, resulting in
a good overall performance.

IParse has a proven track record in many application (including
a commercial application), but it should be noted that some parts
are still under construction, such as the LL1HeapColourParser.
The ParParser, an experimental parallel parser, has poor
performance.

Also RcTransl, a tool for language translation between
Windows resource files, is still under development.

http://www.iwriteiam.nl/MM.html

Compiling
---------

Compiling with g++ (version 7.5) in `software` folder:
```
g++ -fno-operator-names all_IParse.cpp -o IParse
```

Compiling with clang (version 6.0) in `software` folder:
```
clang++ -fno-operator-names all_IParse.cpp -o IParse
```

On Windows use Visual C++ 2008 Express Edition with
IParse.sln file.

Testing
-------

On Linux in root folder:

```
software/IParse software/c.gr others/scan.pc -p scan_pc_output
diff scan_pc_output others/scan_pc_output
software/IParse software/c.gr others/scan.pc -unparse unparse_scan.pc
diff unparse_scan.pc others/unparse_scan.pc
```

The diff should not find any differences.

MarkDownC
---------

MarkDownC is a tool for performing [literate programming](https://en.wikipedia.org/wiki/Literate_programming)
with MarkDown files like the ones that are supported by GitHub. The idea is
that you give this program a list of MarkDown files with fragments of C code
and that the program figures out how to put these fragments together in a single
C file, such that the file can be compiled. (I wrote about the conception of this
idea in the blog article [Literate programming with Markdown](http://www.iwriteiam.nl/D2101.html#13).)
For examples on how to use the program, see:
* [Test of the simple parser](https://github.com/FransFaase/RawParser/blob/master/docs/simple_parser_test.md#testing-with-markdownc)
* [Test of the caching parser](https://github.com/FransFaase/RawParser/blob/master/docs/simple_parser_test.md#testing-with-markdownc)

Issue the following command to build the MarkDownC processor:
```
cd software
g++ -fno-operator-names all_IParse.cpp -o IParse
./IParse c_md.gr -o MarkDownCGrammar.cpp
g++ -g -fno-operator-names -Wall MarkDownC.cpp -o MarkDownC
```
Or:
```
cd software
clang++ -fno-operator-names all_IParse.cpp -o IParse
./IParse c_md.gr -o MarkDownCGrammar.cpp
clang++ -g -fno-operator-names -Wall MarkDownC.cpp -o MarkDownC
```




