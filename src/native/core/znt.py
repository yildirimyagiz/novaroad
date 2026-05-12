"""
Nova Native Transpiler (znt)
AST'yi yüksek performanslı C++ koduna dönüştürür.
"""

from src.ast_nodes import *

class NativeTranspiler:
    def __init__(self):
        self.indent_level = 0
        self.output = []
        self.include_headers()

    def include_headers(self):
        self.emit("#include \"z_rt.hpp\"")
        self.emit("")

    def emit(self, code):
        self.output.append("    " * self.indent_level + code)

    def generate(self, node):
        if isinstance(node, Program):
            for stmt in node.statements:
                self.generate(stmt)
        
        elif isinstance(node, StructDef):
            self.emit(f"struct {node.name} {{")
            self.indent_level += 1
            for i, field in enumerate(node.fields):
                cpp_type = self.to_cpp_type(field.type_annotation)
                self.emit(f"{cpp_type} {field.name};")
            
            # Constructor
            args = ", ".join([f"{self.to_cpp_type(f.type_annotation)} _{f.name}" for f in node.fields])
            self.emit(f"{node.name}({args}) : " + ", ".join([f"{f.name}(_{f.name})" for f in node.fields]) + " {}")
            
            self.indent_level -= 1
            self.emit("};")
            self.emit("")

        elif isinstance(node, FunctionDef):
            ret_type = self.to_cpp_type(node.return_type) if node.return_type else "void"
            if node.name == "main":
                self.emit("int main() {")
            else:
                params = ", ".join([f"{self.to_cpp_type(p.type_annotation)} {p.name}" for p in node.parameters])
                self.emit(f"{ret_type} {node.name}({params}) {{")
            
            self.indent_level += 1
            for stmt in node.body:
                self.generate(stmt)
            
            if node.name == "main":
                self.emit("return 0;")
            self.indent_level -= 1
            self.emit("}")
            self.emit("")

        elif isinstance(node, VariableDeclaration):
            cpp_type = self.to_cpp_type(node.type_annotation) if node.type_annotation else "auto"
            value = self.gen_expr(node.value) if node.value else ""
            self.emit(f"{cpp_type} {node.name} = {value};")

        elif isinstance(node, ForStatement):
            iterable = self.gen_expr(node.iterable)
            self.emit(f"for (auto& {node.variable} : {iterable}) {{")
            self.indent_level += 1
            for stmt in node.body:
                self.generate(stmt)
            self.indent_level -= 1
            self.emit("}")

        elif isinstance(node, IfStatement):
            self.emit(f"if ({self.gen_expr(node.condition)}) {{")
            self.indent_level += 1
            for stmt in node.then_block: self.generate(stmt)
            self.indent_level -= 1
            self.emit("}")
            if node.else_block:
                self.emit("else {")
                self.indent_level += 1
                for stmt in node.else_block: self.generate(stmt)
                self.indent_level -= 1
                self.emit("}")

        elif isinstance(node, Assignment):
             target = self.gen_expr(node.target)
             value = self.gen_expr(node.value)
             self.emit(f"{target} = {value};")

        elif isinstance(node, ExpressionStatement):
             expr_str = self.gen_expr(node.expression)
             if expr_str: self.emit(expr_str + ";")

    def gen_expr(self, expr):
        if isinstance(expr, IntegerLiteral): return str(expr.value)
        if isinstance(expr, FloatLiteral): return str(expr.value)
        if isinstance(expr, StringLiteral): 
            escaped = expr.value.replace("\n", "\\n").replace("\"", "\\\"")
            return f"str(\"{escaped}\")"
        if isinstance(expr, Identifier): return expr.name
        if isinstance(expr, MemberAccess):
            return f"{self.gen_expr(expr.object)}.{expr.member}"
        if isinstance(expr, FunctionCall):
            name = expr.name
            args_list = [self.gen_expr(a) for a in expr.arguments]
            if name in ("console.log", "print", "log"):
                args = " << ".join(args_list)
                return f"std::cout << {args} << std::endl"
            return f"{name}({', '.join(args_list)})"

        if isinstance(expr, Call):
            # Special case: obj.to_string()
            if isinstance(expr.target, MemberAccess) and expr.target.member == "to_string":
                obj = self.gen_expr(expr.target.object)
                return f"std::to_string({obj})"
                
            target = self.gen_expr(expr.target)
            args_list = [self.gen_expr(a) for a in expr.arguments]
            if target == "console.log":
                args = " << ".join(args_list)
                return f"std::cout << {args} << std::endl"
            return f"{target}({', '.join(args_list)})"
        if isinstance(expr, BinaryOp):
            return f"({self.gen_expr(expr.left)} {expr.operator} {self.gen_expr(expr.right)})"
        if isinstance(expr, Cast):
            return f"static_cast<{self.to_cpp_type(expr.target_type)}>({self.gen_expr(expr.expression)})"
        if isinstance(expr, ArrayLiteral):
            if not expr.elements: return "std::vector<int>{}"
            first_elem = expr.elements[0]
            # Simple heuristic for common types
            elem_type = "auto"
            if isinstance(first_elem, FunctionCall) and first_elem.name == "LanguageBaseline":
                elem_type = "LanguageBaseline"
            elif isinstance(first_elem, IntegerLiteral): elem_type = "i32"
            elif isinstance(first_elem, FloatLiteral): elem_type = "f64"
            elif isinstance(first_elem, StringLiteral): elem_type = "str"
            
            elements = ", ".join([self.gen_expr(e) for e in expr.elements])
            return f"std::vector<{elem_type}>{{ {elements} }}"
        return "/* unknown expr */"

    def to_cpp_type(self, zen_type):
        mapping = {
            'i32': 'i32', 'i64': 'int64_t',
            'u32': 'u32', 'u64': 'uint64_t',
            'f64': 'f64', 'str': 'str', 'void': 'void'
        }
        return mapping.get(zen_type, zen_type)

    def get_code(self):
        return "\n".join(self.output)
