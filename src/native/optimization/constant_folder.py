#!/usr/bin/env python3
"""
Constant Folding Optimizer for Nova
Evaluates constant expressions at compile-time
"""

import ast
from typing import Any, Union


class ConstantFolder:
    """
    Fold constant expressions at compile-time
    
    Examples:
        2 + 3 → 5
        "hello" + " world" → "hello world"
        10 * 5 + 2 → 52
        True and False → False
    """
    
    def __init__(self):
        self.folded_count = 0
        
    def fold(self, node):
        """Main folding entry point"""
        if node is None:
            return node
        
        # Handle different node types
        if isinstance(node, dict):
            return self.fold_dict(node)
        elif isinstance(node, list):
            return [self.fold(item) for item in node]
        else:
            return node
    
    def fold_dict(self, node: dict):
        """Fold dictionary-based AST nodes"""
        node_type = node.get('type')
        
        if node_type == 'BinaryOp':
            return self.fold_binary_op(node)
        
        elif node_type == 'UnaryOp':
            return self.fold_unary_op(node)
        
        elif node_type == 'Literal':
            return node  # Already constant
        
        elif node_type == 'FunctionDef':
            # Fold function body
            node['body'] = [self.fold(stmt) for stmt in node.get('body', [])]
            return node
        
        elif node_type == 'If':
            # Fold condition
            node['condition'] = self.fold(node.get('condition'))
            
            # If condition is constant, eliminate dead branch
            if self.is_constant(node['condition']):
                cond_value = self.evaluate_constant(node['condition'])
                if cond_value:
                    # True branch - eliminate else
                    return self.fold(node.get('then_branch'))
                else:
                    # False branch - eliminate then
                    return self.fold(node.get('else_branch'))
            
            node['then_branch'] = self.fold(node.get('then_branch'))
            node['else_branch'] = self.fold(node.get('else_branch'))
            return node
        
        elif node_type == 'While':
            # Fold condition
            node['condition'] = self.fold(node.get('condition'))
            node['body'] = [self.fold(stmt) for stmt in node.get('body', [])]
            return node
        
        elif node_type == 'For':
            node['body'] = [self.fold(stmt) for stmt in node.get('body', [])]
            return node
        
        else:
            # Recursively fold children
            for key, value in node.items():
                if isinstance(value, (dict, list)):
                    node[key] = self.fold(value)
            return node
    
    def fold_binary_op(self, node: dict):
        """Fold binary operations"""
        left = self.fold(node['left'])
        right = self.fold(node['right'])
        op = node['op']
        
        # Update node with folded children
        node['left'] = left
        node['right'] = right
        
        # If both are constants, evaluate
        if self.is_constant(left) and self.is_constant(right):
            left_val = self.evaluate_constant(left)
            right_val = self.evaluate_constant(right)
            
            try:
                result = self.evaluate_binary_op(left_val, op, right_val)
                self.folded_count += 1
                
                # Return as literal
                return {
                    'type': 'Literal',
                    'value': result,
                    'value_type': type(result).__name__
                }
            except Exception as e:
                # Cannot fold (e.g., division by zero)
                return node
        
        return node
    
    def fold_unary_op(self, node: dict):
        """Fold unary operations"""
        operand = self.fold(node['operand'])
        op = node['op']
        
        node['operand'] = operand
        
        if self.is_constant(operand):
            operand_val = self.evaluate_constant(operand)
            
            try:
                result = self.evaluate_unary_op(op, operand_val)
                self.folded_count += 1
                
                return {
                    'type': 'Literal',
                    'value': result,
                    'value_type': type(result).__name__
                }
            except Exception:
                return node
        
        return node
    
    def is_constant(self, node) -> bool:
        """Check if node is a constant"""
        if not isinstance(node, dict):
            return False
        
        return node.get('type') == 'Literal'
    
    def evaluate_constant(self, node) -> Any:
        """Get constant value from node"""
        if not isinstance(node, dict):
            return None
        
        return node.get('value')
    
    def evaluate_binary_op(self, left: Any, op: str, right: Any) -> Any:
        """Evaluate binary operation"""
        if op == '+':
            return left + right
        elif op == '-':
            return left - right
        elif op == '*':
            return left * right
        elif op == '/':
            if isinstance(left, int) and isinstance(right, int):
                return left // right  # Integer division
            return left / right
        elif op == '%':
            return left % right
        elif op == '**':
            return left ** right
        elif op == '==':
            return left == right
        elif op == '!=':
            return left != right
        elif op == '<':
            return left < right
        elif op == '<=':
            return left <= right
        elif op == '>':
            return left > right
        elif op == '>=':
            return left >= right
        elif op == 'and':
            return left and right
        elif op == 'or':
            return left or right
        else:
            raise ValueError(f"Unknown operator: {op}")
    
    def evaluate_unary_op(self, op: str, operand: Any) -> Any:
        """Evaluate unary operation"""
        if op == '-':
            return -operand
        elif op == 'not':
            return not operand
        elif op == '+':
            return +operand
        else:
            raise ValueError(f"Unknown unary operator: {op}")
    
    def get_statistics(self) -> dict:
        """Get folding statistics"""
        return {
            'folded_expressions': self.folded_count
        }


def example_usage():
    """Example of constant folding"""
    
    # Example AST: 2 + 3 * 4
    ast_tree = {
        'type': 'BinaryOp',
        'op': '+',
        'left': {'type': 'Literal', 'value': 2},
        'right': {
            'type': 'BinaryOp',
            'op': '*',
            'left': {'type': 'Literal', 'value': 3},
            'right': {'type': 'Literal', 'value': 4}
        }
    }
    
    folder = ConstantFolder()
    folded = folder.fold(ast_tree)
    
    print("Original AST: 2 + 3 * 4")
    print("Folded AST:", folded)
    # Result: {'type': 'Literal', 'value': 14}
    print(f"Folded {folder.folded_count} expressions")


if __name__ == '__main__':
    example_usage()
