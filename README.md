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

Also RcTransl, an tool for language translation between
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

On Windows use A Visual C++ 2008 Express Edition with
IParse.sln file.

Testing
-------

On Linux in root folder:

```
software/IParse c.gr others/scan.pc -p scan_pc_output
diff scan_pc_output others/scan_pc_output
software/IParse software/c.gr others/scan.pc -unparse others/unparse_scan.pc
diff unparse_scan.pc others/unparse_scan.pc
```

The diff should not find any differences.

