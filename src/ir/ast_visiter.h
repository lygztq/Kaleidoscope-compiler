#ifndef KALEIDOSCOPE_IR_AST_VISITER_H_
#define KALEIDOSCOPE_IR_AST_VISITER_H_

#include <memory>
#include <unordered_map>

#include "kaleidoscope/ast.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

namespace kaleidoscope {
namespace ir {

class AstLLVMCodeGen {
 public:
  llvm::Value* visit(ast::NumberExprAST* number_ptr);
  llvm::Value* visit(ast::VariableExprAST* var_ptr);
  llvm::Value* visit(ast::BinaryExprAST* bin_ptr);
  llvm::Value* visit(ast::CallExprAST* call_ptr);
  llvm::Value* visit(ast::ExprAST* bin_ptr);
  llvm::Function* visit(ast::FunctionAST* func_ptr);
  llvm::Function* visit(ast::ProtoTypeAST* proto_ptr);

 private:
  llvm::LLVMContext context_;
  llvm::IRBuilder<> builder_;
  std::unique_ptr<llvm::Module> module_;
  std::unordered_map<std::string, llvm::Value*> named_values;
};

}  // namespace ir
}  // namespace kaleidoscope

#endif  // KALEIDOSCOPE_IR_AST_VISITER_H_
