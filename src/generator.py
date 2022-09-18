#!/usr/bin/env python3

import sys

def main(path):
    shader_sections = []
    try:
        with open(path + "shaders.glsl") as f:
            sections = f.read().split('$')
            if len(sections) <= 1:
                print("No sections found in " + path + "shaders.glsl")
                return
            for s in sections[1:]:
                if len(s) > 1:
                    lines = s.splitlines()
                    if len(lines) > 1:
                        shader_sections.append(lines)
    except FileNotFoundError:
        print("Could not find " + path + "shaders.glsl")
        return

    output = "#pragma once\n\n#include <stdio.h>\n\n"

    output += "static int _copy_string(char *dst, const char *src) {\n"
    output += "\tint len = 0;\n"
    output += "\twhile ((*dst++ = *src++)) len++;\n"
    output += "\treturn len;\n}\n\n"

    for shader in shader_sections:
        header = shader[0].split(" ")
        name = header[0]
        extra_len = 0 if len(header) <= 1 else int(header[1])
        total_len = extra_len
        params = []
        parts = []
        cur_part = ""
        for line in shader[1:]:
            cur_part += "\t\t\""
            prev = None
            for c in line:
                if prev == '@':
                    valid = True
                    if c == 'f':
                        params.append("float")
                    elif c == 'i':
                        params.append("int")
                    elif c == 'm':
                        params.append("multi")
                    else:
                        valid = False
                    if valid:
                        cur_part += '"'
                        parts.append(cur_part)
                        cur_part = "\t\t\""
                elif c != '@':
                    if c == '"':
                        cur_part += '\\'
                    cur_part += c
                prev = c
                total_len += 1
            cur_part += "\\n\"\n"
            total_len += 1

        parts.append(cur_part)

        buffer_name = "_{0}_text".format(name)
        output += "static char {0}[{1}];\n\n".format(buffer_name, total_len + 1)
        output += "static const char *make_" + name + "_shader("
        i = 0
        while i < len(params):
            if i > 0:
                output += ", "
            
            if params[i][0] == 'm':
                output += "const char **arg{0}, int arg{0}_len".format(i)
            else:
                output += "{0} arg{1}".format(params[i], i)
            i += 1

        output += ") {\n"
        output += "\tint offset = 0;\n"
        i = 0
        for part in parts:
            output += "\n\toffset += _copy_string(" + buffer_name + " + offset,\n" + part + "\n\t);\n"
            if i < len(parts)-1:
                if params[i][0] == 'm':
                    output += "\n\tfor (int i = 0; i < arg{}_len; i++)".format(i)
                    output += "\n\t\toffset += _copy_string({0} + offset, arg{1}[i]);\n".format(buffer_name, i)
                else:
                    output += "\toffset += sprintf({0} + offset, \"%{1}\", arg{2});\n".format(buffer_name, params[i][0], i)
            i += 1

        output += "\n\treturn &" + buffer_name + "[0];\n}\n\n"

    with open(path + "shaders.h", "w") as f:
        f.write(output)

path = "." if len(sys.argv) < 2 else sys.argv[1]
main(path + "/")