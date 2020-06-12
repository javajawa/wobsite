#!/usr/bin/python3

# SPDX-FileCopyrightText: 2020 Benedict Harcourt <ben.harcourt@harcourtprogramming.co.uk>
#
# SPDX-License-Identifier: BSD-2-Clause

# vim: tabstop=4 expandtab nospell

from __future__ import annotations

from string import ascii_letters
from typing import Any, Dict, TextIO

ASCII_AND_NULL = set(ascii_letters).union(set("\0"))
Tree = Dict[str, Any]


def add_token(node: Tree, token: str, fulltoken: str) -> None:
    if not token:
        node["\0"] = fulltoken
        return

    if not token[0] in node:
        node[token[0]] = {}

    if not isinstance(node[token[0]], Dict):
        raise Exception("Invalid state")

    add_token(node[token[0]], token[1:], fulltoken)


def print_token_read(indent: int, token: str, char: int, c_file: TextIO) -> None:
    c_file.write("\n" + "\t" * indent + "str += " + str(char) + ";\n")
    c_file.write("\n" + "\t" * indent + "while ( str[0] == ' ' ) ++str;\n")
    c_file.write("\n" + "\t" * indent + "out->" + token + " = str;\n")


def print_tree(node: Tree, indent: int, char: int, c_file: TextIO) -> None:
    if len(node) == 1:
        for i in sorted(node.keys()):
            if i == "\0":
                print_token_read(indent, node[i], char, c_file)
                return

            if i in ascii_letters:
                format_str = "if ( ( str[{0}] & 0xDF ) != '{1}' )"
            else:
                format_str = "if (   str[{0}]          != '{1}' )"

            w(c_file, indent, format_str.format(char, i.upper()), "{")
            w(c_file, indent + 1, "str += {0};".format(char), "goto cleanup;")
            w(c_file, indent, "}")

            print_tree(node[i], indent, char + 1, c_file)

        return

    case = set(node.keys()).issubset(ASCII_AND_NULL)

    if case:
        format_str = "switch ( str[{0}] & 0xDF )"
    else:
        format_str = "switch ( str[{0}] )"

    w(c_file, indent, "", format_str.format(char), "{")

    for i in sorted(node.keys()):
        if i == "\0":
            w(c_file, indent + 1, "case '\\0':")
            print_token_read(indent + 2, node[i], char, c_file)

        else:
            w(c_file, indent + 1, "case '{0}':".format(i.upper()))

            if not case and i.lower() != i.upper():
                w(c_file, indent + 1, "case '{0}':".format(i.lower()))

            print_tree(node[i], indent + 2, char + 1, c_file)

        w(c_file, indent + 2, "break;", "")

    w(c_file, indent + 1, "default:")
    w(c_file, indent + 2, "goto cleanup;")
    w(c_file, indent, "}", "")


def gen(name: str) -> None:
    # Tree contains each of the per letter parse states,
    # encoded as [ current state + letter => new state ].
    # a 'letter' which is the null character represents
    # a termination of a token.
    tree: Tree = {}

    # Set up the output files
    h_file: TextIO = open(name + ".h", "w")
    c_file: TextIO = open(name + ".c", "w")

    # Read from the input file
    with open(name + ".fl", "rU") as input_file:
        # The first two lines of the file are the seperator
        # between key and value,
        sep = input_file.readline().strip()
        term = input_file.readline().strip()

        if input_file.readline() != "\n":
            raise Exception("Line 3 of input file should be blank.")

        h_file.write("struct " + "lexed_" + name + "\n{\n")

        for line in input_file:
            if line.strip():
                h_file.write("\tconst char * " + line.strip().replace("-", "_") + ";\n")
                add_token(tree, line.strip() + sep, line.strip().replace("-", "_"))

        h_file.write("};\n\n")
        h_file.write(
            "void lex_"
            + name
            + "( char * restrict str, struct lexed_"
            + name
            + " * restrict out );\n"
        )

    h_file.close()

    w(c_file, 0, '#include "' + name + '.h"', "#include <string.h>", "")
    w(
        c_file,
        0,
        "void lex_"
        + name
        + "( char * restrict str, struct lexed_"
        + name
        + " * restrict out )",
        "{",
    )

    w(c_file, 1, "memset( out, 0, sizeof( struct lexed_" + name + " ) );")

    w(c_file, 0, "")
    w(c_file, 1, "while ( 1 )", "{")

    print_tree(tree, 2, 0, c_file)

    w(c_file, 0, "", "cleanup:")
    w(c_file, 2, "while ( 1 )", "{")
    w(c_file, 3, "if ( str[0] == 0 )", "{")
    w(c_file, 4, "return;")
    w(c_file, 3, "}", "")
    w(c_file, 3, "if ( str[0] == '" + term + "' )", "{")
    w(c_file, 4, "str[0] = 0;", "++str;", "break;")
    w(c_file, 3, "}")

    if term == "\\n":
        w(c_file, 3, "", "if ( str[0] == '\\r' && str[1] == '\\n' )", "{")
        w(c_file, 4, "str[0] = 0;", "str[1] = 0;", "str += 2;", "break;")
        w(c_file, 3, "}")

    w(c_file, 3, "", "++str;")

    w(c_file, 2, "}")
    w(c_file, 1, "}")
    w(c_file, 0, "}")

    c_file.close()


def w(file: TextIO, indent: int, *data: str) -> None:
    for line in data:
        if line:
            file.write("\t" * indent + line + "\n")
        else:
            file.write("\n")


if __name__ == "__main__":
    gen("headers")
