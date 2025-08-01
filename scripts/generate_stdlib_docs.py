#!/usr/bin/env python3
"""
Python script to generate function and macro documentation in stdlib.md
"""

import glob
import os
import sys
import re
from typing import Optional

class Param:
    name: str = ""
    type: str = "none"
    description: str = ""

class LastExpr:
    type: str = "none"
    description: str = ""

class Macro:
    name: str = ""
    params: list[Param] = None
    last_expr: LastExpr = LastExpr()
    summary: str = ""
    description: str = ""

    def __init__(self):
      self.params = []

def parse_param_string(param_string: str) -> Optional[Param]:
    """
    Parse a parameter string in the format:
    ":param (type) $name: description"
    """
    pattern = r':param\s+\(([^)]+)\)\s+((\$|@)[^:]+):\s*(.+)'

    match = re.match(pattern, param_string.strip())
    if match:
        param = Param()
        param.type = match.group(1).strip()
        param.name = match.group(2).strip()
        param.description = match.group(4).strip()
        return param

    return None

def parse_last_expr_string(param_string: str) -> Optional[LastExpr]:
    """
    Parse a parameter string in the format:
    ":last_expr (type): description"
    """
    pattern = r':last_expr\s+\(([^)]+)\):\s*(.+)'

    match = re.match(pattern, param_string.strip())
    if match:
        last_expr = LastExpr()
        last_expr.type = match.group(1).strip()
        last_expr.description = match.group(2).strip()
        return last_expr

    return None

def parse_macro_name(macro_string: str) -> Optional[str]:
    # Pattern to match "macro name("
    pattern = r'macro\s+([^(]+)\('

    match = re.match(pattern, macro_string.strip())
    if match:
        return match.group(1).strip()

    return None

def read_file_lines(file_path: str) -> Optional[list[Macro]]:
    macros: list[Macro] = []
    try:
        if not os.path.exists(file_path):
            print(f"Error: File '{file_path}' not found.")
            return None

        current = Macro()
        with open(file_path, 'r', encoding='utf-8') as file:
            for line in file:
                line_content = line.lstrip().rstrip()
                if line_content.startswith("//"):
                    # Don't strip, because we may have markdown with meaningful
                    # whitespace at the beginning of the line that we preserve.
                    line_content = line_content[2:]
                    if len(line_content) > 0 and line_content[:1].isspace():
                        line_content = line_content[1:]
                    if line_content.startswith(":param"):
                        parsed_param = parse_param_string(line_content)
                        if parsed_param:
                            current.params.append(parsed_param)
                    elif line_content.startswith(":last_expr"):
                        parsed_last_expr = parse_last_expr_string(line_content)
                        if parsed_last_expr:
                            current.last_expr = parsed_last_expr
                    elif not current.summary:
                        current.summary = line_content
                    elif current.description or line_content:
                        current.description += line_content + "\n"
                elif line_content.startswith("macro"):
                    current.name = parse_macro_name(line_content)
                    if current.name:
                        # There must at least be a description or it's an
                        # undocumented macro.
                        if current.summary:
                            macros.append(current)
                        else:
                            print(f"Warning: Macro '{current.name}' will not be added to the docs.")
                else:
                    current = Macro()

        return macros

    except PermissionError:
        print(f"Error: Permission denied to read '{file_path}'.")
        return None
    except UnicodeDecodeError:
        print(f"Error: Unable to decode '{file_path}' as UTF-8.")
        return None
    except Exception as e:
        print(f"Error reading file '{file_path}': {e}")
        return None

def write_markdown_doc(macros: list[Macro]):
    print("Writing markdown file to docs/stdlib.md")

    current_lines = []
    with open("docs/stdlib.md", "r", encoding="utf-8") as file:
        current_lines = file.readlines()

    in_macro_section = False
    cleaned_lines = []
    for raw_line in current_lines:
        line = raw_line.strip()
        if line == "## Macros":
            in_macro_section = True
        elif line == "## Map Functions":
            in_macro_section = False
        elif in_macro_section:
            continue

        cleaned_lines.append(raw_line)

    updated_lines = []
    for cleaned_line in cleaned_lines:
        line = cleaned_line.strip()
        if line == "## Macros":
            updated_lines.append(cleaned_line)
            updated_lines.append("\n")

            # Make the table
            updated_lines.append("| Name | Description |\n")
            updated_lines.append("| --- | --- |\n")
            for macro in macros:
                updated_lines.append(f"| [`{macro.name}`](#{macro.name}) | {macro.summary} |\n")
            updated_lines.append("\n")

            # Make the macro details
            for macro in macros:
                updated_lines.append(f"### {macro.name}\n")
                updated_lines.append(f"{macro.summary}\n")
                updated_lines.append("\n")
                if macro.description:
                  updated_lines.append(f"{macro.description}\n")
                updated_lines.append("#### Parameters\n")
                for param in macro.params:
                    updated_lines.append(f"- **{param.name}**: ({param.type}) {param.description}\n")
                updated_lines.append("\n")
                updated_lines.append("#### Last Expression\n")
                if macro.last_expr.type == "none":
                    updated_lines.append("- **None**\n")
                else:
                    updated_lines.append(f"- **{macro.last_expr.type}**: {macro.last_expr.description}\n")
                updated_lines.append("\n")
        else:
            updated_lines.append(cleaned_line)

    with open("docs/stdlib.md", "w", encoding="utf-8") as file:
        file.writelines(updated_lines)

def main():
    stdlib_files = glob.glob("src/stdlib/**/*.bt", recursive=True)

    if len(stdlib_files) == 0:
        print("Didn't find any stdlib files")
        sys.exit(1)
    else:
        print(f"All stdlib files: {stdlib_files}")

    all_macros: list[Macro] = []
    for file in stdlib_files:
        macros = read_file_lines(file)
        if macros:
            all_macros += macros

    if len(all_macros) > 0:
        write_markdown_doc(all_macros)
    else:
        sys.exit(1)

if __name__ == "__main__":
    main()
