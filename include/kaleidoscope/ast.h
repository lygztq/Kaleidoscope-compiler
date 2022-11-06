#ifndef KALEIDOSCOPE_AST_H_
#define KALEIDOSCOPE_AST_H_

#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace kaleidoscope {
namespace ast {

enum class ASTType {
  kExpr,
  kPrototype,
  kFunction,
};

class AST {
 public:
  using Ptr = std::unique_ptr<AST>;

  AST() = delete;
  explicit AST(ASTType type) : type_(type) {}
  virtual ~AST() {}

  ASTType GetType() const { return this->type_; }

  virtual void Dump(std::ostream& sm) const {
    sm << "[general ast with type_id: " << static_cast<int>(type_) << "]";
  }

  friend std::ostream& operator<<(std::ostream& sm, const AST& ast) {
    ast.Dump(sm);
    return sm;
  }

 private:
  ASTType type_;
};

class ExprAST : public AST {
 public:
  ExprAST() : AST(ASTType::kExpr) {}
};

class NumberExprAST : public ExprAST {
 public:
  NumberExprAST() : ExprAST(), value_(0) {}
  explicit NumberExprAST(double val) : ExprAST(), value_(val) {}

  virtual void Dump(std::ostream& sm) const override { sm << value_; }
  double GetValue() const { return value_; }

 private:
  double value_;
};

class VariableExprAST : public ExprAST {
 public:
  VariableExprAST() = default;
  explicit VariableExprAST(const std::string& name) : ExprAST(), name_(name) {}

  virtual void Dump(std::ostream& sm) const override { sm << "%" << name_; }
  const std::string& GetName() const { return name_; }

 private:
  std::string name_;
};

enum class SupportBinaryOpTag { kAdd, kSub, kMul, kDiv, kLess, kInvalid };

void StringToBinaryOpTagInit(
    std::unordered_map<std::string, SupportBinaryOpTag>& map) {
  map["+"] = SupportBinaryOpTag::kAdd;
  map["-"] = SupportBinaryOpTag::kSub;
  map["*"] = SupportBinaryOpTag::kMul;
  map["/"] = SupportBinaryOpTag::kDiv;
  map["<"] = SupportBinaryOpTag::kLess;
}

std::string BinoryOpTagName(SupportBinaryOpTag tag) {
  switch (tag) {
    case SupportBinaryOpTag::kAdd:
      return "+";
    case SupportBinaryOpTag::kSub:
      return "-";
    case SupportBinaryOpTag::kMul:
      return "*";
    case SupportBinaryOpTag::kDiv:
      return "/";
    case SupportBinaryOpTag::kLess:
      return "<";
  }
  return "";
}

inline SupportBinaryOpTag StringToBinaryOpTag(const std::string& str_literal) {
  thread_local /*static*/ std::unordered_map<std::string, SupportBinaryOpTag>
      map_;
  std::once_flag flag;
  std::call_once(flag, StringToBinaryOpTagInit, map_);
  return (map_.count(str_literal) == 0) ? SupportBinaryOpTag::kInvalid
                                        : map_[str_literal];
}

class BinaryExprAST : public ExprAST {
 public:
  BinaryExprAST() = delete;
  BinaryExprAST(const std::string& op_literal, std::unique_ptr<ExprAST> lhs,
                std::unique_ptr<ExprAST> rhs)
      : ExprAST(),
        op_tag_(StringToBinaryOpTag(op_literal)),
        lhs_(std::move(lhs)),
        rhs_(std::move(rhs)) {}
  BinaryExprAST(SupportBinaryOpTag op_tag, std::unique_ptr<ExprAST> lhs,
                std::unique_ptr<ExprAST> rhs)
      : ExprAST(),
        op_tag_(op_tag),
        lhs_(std::move(lhs)),
        rhs_(std::move(rhs)) {}

  virtual void Dump(std::ostream& sm) const override {
    sm << "(";
    lhs_->Dump(sm);
    sm << ") " << BinoryOpTagName(op_tag_) << " (";
    rhs_->Dump(sm);
    sm << ")";
  }

  ExprAST* GetLHS() { return lhs_.get(); }
  const ExprAST* GetLHS() const { return lhs_.get(); }
  ExprAST* GetRHS() { return rhs_.get(); }
  const ExprAST* GetRHS() const { return rhs_.get(); }
  const auto GetOpTag() { return op_tag_; }

 private:
  SupportBinaryOpTag op_tag_;
  std::unique_ptr<ExprAST> lhs_, rhs_;
};

class CallExprAST : public ExprAST {
 public:
  CallExprAST() = delete;
  CallExprAST(const std::string& callee,
              std::vector<std::unique_ptr<ExprAST>> args)
      : ExprAST(), callee_(callee), args_(std::move(args)) {}

  virtual void Dump(std::ostream& sm) const override {
    sm << callee_ << "(";
    if (!args_.empty()) {
      auto iter = args_.cbegin();
      (*iter)->Dump(sm);
      ++iter;
      while (iter != args_.cend()) {
        sm << ", ";
        (*iter)->Dump(sm);
        ++iter;
      }
    }
    sm << ")";
  }

  const std::string& GetCallee() const { return callee_; }
  const auto& GetArgs() const { return args_; }
  const auto& GetArg(int idx) const { return args_[idx]; }

 private:
  std::string callee_;
  std::vector<std::unique_ptr<ExprAST>> args_;
};

class ProtoTypeAST : public AST {
 public:
  ProtoTypeAST() = delete;
  ProtoTypeAST(const std::string& name, std::vector<std::string> args)
      : AST(ASTType::kPrototype), name_(name), args_(std::move(args)) {}

  const std::string& GetName() const { return name_; }

  virtual void Dump(std::ostream& sm) const override {
    sm << name_ << "(";
    if (!args_.empty()) {
      auto iter = args_.cbegin();
      sm << (*iter);
      ++iter;
      while (iter != args_.cend()) {
        sm << ", ";
        sm << (*iter);
        ++iter;
      }
    }
    sm << ")\n";
  }

  const std::string& GetName() const { return name_; }
  const std::vector<std::string>& GetArgs() const { return args_; }

 private:
  std::string name_;
  std::vector<std::string> args_;
};

class FunctionAST : public AST {
 public:
  FunctionAST() = delete;
  FunctionAST(std::unique_ptr<ProtoTypeAST> prototype,
              std::unique_ptr<ExprAST> body)
      : AST(ASTType::kFunction),
        prototype_(std::move(prototype)),
        body_(std::move(body)) {}

  virtual void Dump(std::ostream& sm) const override {
    prototype_->Dump(sm);
    sm << "{\n";
    body_->Dump(sm);
    sm << "\n}\n";
  }

  ProtoTypeAST* GetProto() { return prototype_.get(); }
  ExprAST* GetBody() { return body_.get(); }

 private:
  std::unique_ptr<ProtoTypeAST> prototype_;
  std::unique_ptr<ExprAST> body_;
};

}  // namespace ast
}  // namespace kaleidoscope

#endif  // KALEIDOSCOPE_AST_H_
