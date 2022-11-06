#include <vector>
#include "ast_visiter.h"

#include "kaleidoscope/logging.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Verifier.h"

namespace kaleidoscope {
namespace ir {

#define CodeGenError(msg)            \
  {                                  \
    LOG_WARNING << msg << std::endl; \
    return nullptr;                  \
  }

llvm::Value* AstLLVMCodeGen::visit(ast::NumberExprAST* number_ptr) {
  return llvm::ConstantFP::get(context_, llvm::APFloat(number_ptr->GetValue()));
}

llvm::Value* AstLLVMCodeGen::visit(ast::VariableExprAST* var_ptr) {
  // look this variable up in the function
  llvm::Value* var = named_values[var_ptr->GetName()];
  if (!var) {
    CodeGenError("Unkonwn variable name");
  }
  return var;
}

llvm::Value* AstLLVMCodeGen::visit(ast::BinaryExprAST* bin_ptr) {
  llvm::Value* left_value = visit(bin_ptr->GetLHS());
  llvm::Value* right_value = visit(bin_ptr->GetRHS());
  if (!left_value || !right_value) {
    return nullptr;
  }

  switch (bin_ptr->GetOpTag()) {
    case ast::SupportBinaryOpTag::kAdd:
      return builder_.CreateFAdd(left_value, right_value, "addtmp");
    case ast::SupportBinaryOpTag::kSub:
      return builder_.CreateFSub(left_value, right_value, "subtmp");
    case ast::SupportBinaryOpTag::kMul:
      return builder_.CreateFMul(left_value, right_value, "multmp");
    case ast::SupportBinaryOpTag::kLess:
      left_value = builder_.CreateFCmpULT(left_value, right_value, "cmptmp");
      // Convert bool 0/1 to double 0.0 or 1.0
      return builder_.CreateUIToFP(
          left_value, llvm::Type::getDoubleTy(context_), "booltmp");
    default:
      CodeGenError("Invalid binary operator");
  }
  return nullptr;
}

llvm::Value* AstLLVMCodeGen::visit(ast::CallExprAST* call_ptr) {
  // look up the name in the global module table.
  llvm::Function* callee_func = module_->getFunction(call_ptr->GetCallee());
  if (!callee_func) {
    CodeGenError("Unknown function referenced");
  }

  // If argument mismatch error
  if (callee_func->arg_size() != call_ptr->GetArgs().size()) {
    CodeGenError("Incorrect # arguments passes");
  }

  std::vector<llvm::Value*> llvm_args;
  for (auto i = 0; i < call_ptr->GetArgs().size(); ++i) {
    llvm_args.push_back(visit(call_ptr->GetArg(i).get()));
    if (!llvm_args.back()) {
      return nullptr;
    }
  }
  return builder_.CreateCall(callee_func, llvm_args, "calltmp");
}

llvm::Value* AstLLVMCodeGen::visit(ast::ExprAST* expr_ptr) {
  CodeGenError("This procedure should not be called directly.");
}

llvm::Function* AstLLVMCodeGen::visit(ast::ProtoTypeAST* proto_ptr) {
  // Make the function type: double(duble...) etc.
  std::vector<llvm::Type*> doubles(proto_ptr->GetArgs().size(), llvm::Type::getDoubleTy(context_));
  auto* func_type = llvm::FunctionType::get(llvm::Type::getDoubleTy(context_), doubles, false);
  auto* func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, proto_ptr->GetName(), module_.get());

  // Set names for all arguments
  unsigned int idx = 0;
  for (auto& arg: func->args()) {
    arg.setName(proto_ptr->GetArgs()[idx++]);
  }

  return func;
}

llvm::Function* AstLLVMCodeGen::visit(ast::FunctionAST* func_ptr) {
  // TODO: use function definition's arg name to overwrite arg name in the previous extern declaration.
  // First, check for an existing function from a previous 'extern' declaration.
  llvm::Function* def_func = module_->getFunction(func_ptr->GetProto()->GetName());
  if (!def_func) {
    def_func = visit(func_ptr->GetProto());
  }

  if (!def_func) {
    return nullptr; // error
  }

  // multi def
  if (!def_func->empty()) {
    CodeGenError("Function cannot be redefined.");
    return nullptr;
  }

  // Create a new basic block to start insertion into.
  llvm::BasicBlock *bb = llvm::BasicBlock::Create(context_, "entry", def_func);
  builder_.SetInsertPoint(bb);

  // Record the function arguments in the named values map
  named_values.clear();
  for (auto &arg : def_func->args()) {
    named_values[static_cast<const std::string>(arg.getName())] = &arg;
  }

  if (llvm::Value* ret_val = visit(func_ptr->GetBody())) {
    // finish off the function
    builder_.CreateRet(ret_val);

    // validate the generated code, checking for consistency
    llvm::verifyFunction(*def_func);
    return def_func;
  }

  // error reading body, remove function
  def_func->eraseFromParent();
  return nullptr;
}

}  // namespace ir
}  // namespace kaleidoscope
