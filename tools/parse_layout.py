#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2020 Benedict Harcourt <ben.harcourt@harcourtprogramming.co.uk>
#
# SPDX-License-Identifier: BSD-2-Clause

# vim: tabstop=4 expandtab

from __future__ import annotations

from typing import Dict, IO

import sys
import re

from os import SEEK_CUR, path
from xml.dom import minidom
from xml.dom.minidom import Node
from html import escape


def create_node(
    handle: IO[bytes], node: Node, tokens: Dict[str, int], depth: int
) -> bool:
    if node.nodeType == Node.ELEMENT_NODE:
        return create_element(handle, node, tokens, depth)

    if node.nodeType == Node.TEXT_NODE:
        return create_text(handle, node, tokens, depth)

    if node.nodeType == Node.CDATA_SECTION_NODE:
        return create_cdata(handle, node, depth)

    handle.write(f"/* {Node} */".encode("utf-8"))
    return False


def create_cdata(handle: IO[bytes], text: Node, depth: int) -> bool:
    if text.data.strip() == "":
        return False

    data: str = re.sub(r"\s+", " ", text.data)

    handle.write(b"\t" * depth)
    handle.write(b"text(")

    handle.write(b"\n" if "\n" in data else b" ")

    handle.write(b'"')
    handle.write(escape(data).encode("utf-8"))
    handle.write(b'"')

    if "\n" in data:
        handle.write(b"\n")
        handle.write(b"\t" * depth)
    else:
        handle.write(b" ")

    handle.write(b")\n")

    return True


def create_text(
    handle: IO[bytes], text: Node, tokens: Dict[str, int], depth: int
) -> bool:
    if text.data.strip() == "":
        return False

    if text.data[0] != "$":
        return create_cdata(handle, text, depth)

    text.data = text.data[1:]
    tokens[text.data] = 1

    handle.write(b"\t" * depth)
    handle.write(b"replace( ")
    handle.write(text.data.encode("utf-8"))
    handle.write(b" )\n")

    return True


def create_element(
    handle: IO[bytes], element: Node, tokens: Dict[str, int], depth: int
) -> bool:
    if element.tagName == "_var":
        return create_text(handle, element.firstChild, tokens, depth)

    # Open a new element() macros
    handle.write(b"\t" * depth)
    handle.write(b"element(\n")

    # First argument -- the tag name of the element
    handle.write(b"\t" * (depth + 1))
    handle.write(f'"{element.tagName}",\n'.encode("utf-8"))

    # Second argument -- list of attributes
    attrs = element.attributes

    handle.write(b"\t" * (depth + 1))

    if attrs:
        # Start the attrs() macro
        handle.write(b"attrs( ")

        for key in attrs.keys():
            val = element.getAttribute(key)

            # Attribute values that start with $ are replacements
            # with the replacement token being the rest of the value.
            if val[0] == "$":
                tokens[val[1:]] = 1
                handle.write(f'at_rep( "{key}", {val[1:]} ), '.encode("utf-8"))

            # Otherwise, the value is taken as a literal string
            else:
                handle.write(f'at_str( "{key}", "{val}" ), '.encode("utf-8"))

        # Each attribute above ends in ", ". To end the macro, we must
        # remove this and put the closing bracket.
        handle.seek(-2, SEEK_CUR)
        handle.write(b" ),\n")

    # No attributes just has this list point to NULL
    else:
        handle.write(b"NULL,\n")

    # Third argument: the list of child nodes
    handle.write(b"\t" * (depth + 1))

    if element.hasChildNodes():
        handle.write(b"children(\n")

        for child in element.childNodes:
            if create_node(handle, child, tokens, depth + 2):
                handle.seek(-1, SEEK_CUR)
                handle.write(b",\n")

        handle.seek(-2, SEEK_CUR)
        handle.write(b"\n")
        handle.write(b"\t" * (depth + 1))
        handle.write(b")\n")
    else:
        handle.write(b"NULL\n")

    # End the element() macro
    handle.write(b"\t" * depth)
    handle.write(b")\n")

    # We create new code
    return True


def parse_layout(layout: str) -> None:
    tree = minidom.parse("layouts/{}.xml".format(layout))

    with open("gen/layouts/{}.c".format(layout), "w+b") as c_file:
        c_file.write(f'#include "gen/layouts/{layout}.h"\n'.encode("utf-8"))
        c_file.write('#include "layout.h"\n\n'.encode("utf-8"))
        c_file.write(f"const union html_node {layout} =\n".encode("utf-8"))

        tokens: Dict[str, int] = {}

        create_node(c_file, tree.documentElement, tokens, 0)

        c_file.seek(-1, SEEK_CUR)
        c_file.write(b";\n")

    with open("include/gen/layouts/{}.h".format(layout), "w") as h_file:
        h_file.write(f"#ifndef layout_{layout}_H\n")
        h_file.write(f"#define layout_{layout}_H\n\n")
        h_file.write('#include "xml/node.h"\n\n')
        h_file.write(f"extern const union html_node {layout};\n\n")

        h_file.write(f"enum {layout}_replacements\n{{\n")

        for token in tokens:
            h_file.write(f"\t{token},\n")

        h_file.write("\t__MAX\n};\n\n#endif\n")


def main(layout: str) -> None:
    layout = path.basename(layout)

    if layout.endswith(".xml"):
        layout = layout[:-4]

    main(layout)


if __name__ == "__main__":
    main(sys.argv[1])
