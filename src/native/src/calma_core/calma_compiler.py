import sys
import os
from pathlib import Path

# Fix paths for imports
sys.path.insert(0, str(Path(__file__).parent.parent.parent.parent))  # Root of nova

from src.lexer import Lexer
from src.parser import Parser
import src.parser_extended
from calma_bridge import CalmaBridge


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 calma_compiler.py <filename.zn>")
        return

    filename = sys.argv[1]
    with open(filename, "r") as f:
        source = f.read()

    # 1. Lexical Analysis
    lexer = Lexer(source)
    tokens = lexer.tokenize()

    # 2. Parsing
    parser = Parser(tokens)
    ast = parser.parse()

    # 3. Transpilation to Calma (C)
    bridge = CalmaBridge()
    bridge.generate(ast)
    c_code = bridge.get_code()

    # 4. Save C File
    output_c = "calma_output.c"
    with open(output_c, "w") as f:
        f.write(c_code)

    print(f"✅ Calma Transpilation complete: {output_c}")
    print(f"🚀 To compile and run:")
    print(f"   clang -O3 -I./include calma_output.c -o calma_app && ./calma_app")


if __name__ == "__main__":
    main()
