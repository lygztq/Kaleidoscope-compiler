#include "kaleidoscope/parser.h"

#include <mutex>
#include <unordered_map>

#include "kaleidoscope/logging.h"

namespace kaleidoscope {
namespace parser {

namespace {

void BinopPrecedenceInit(std::unordered_map<std::string, int>& map) {
  map["<"] = 10;
  map["+"] = 20;
  map["-"] = 20;
  map["*"] = 40;
  map["/"] = 40;
}

int BinopPrecedence(const std::string& op) {
  thread_local static std::unordered_map<std::string, int> precedence;
  std::once_flag init_flag;
  std::call_once(init_flag, BinopPrecedenceInit, precedence);
  if (precedence.find(op) == precedence.end()) {
    return -1;
  } else {
    return precedence[op];
  }
}

}  // namespace

#define PARSE_ERROR_LOG(msg)                                               \
  {                                                                        \
    auto location = current_token_->GetLocation();                         \
    LOG_WARNING << "in source file: " << lexer_.GetSourceFilePath() << ":" \
                << (location.line + 1) << ":" << (location.col + 1)        \
                << ", error message: " << msg << std::endl;                \
  }

void Parser::NextToken() { current_token_ = lexer_.NextToken(); }

std::optional<std::string> Parser::TryGetLexeme(TokenTag tag) const {
  if (tag != current_token_->tag) {
    return std::nullopt;
  }
  switch (tag) {
    case TokenTag::kIdentifier:
    case TokenTag::kPunctuator:
      return std::dynamic_pointer_cast<Word>(current_token_)->lexeme;
    default:
      return std::nullopt;
  }
  return std::nullopt;
}

Parser::NumberExprASTPtr Parser::NumberExprAST() {
  if (current_token_->tag != TokenTag::kNumber) {
    PARSE_ERROR_LOG("expect a number here.");
    return nullptr;
  }
  auto number = std::make_unique<ast::NumberExprAST>(
      std::dynamic_pointer_cast<Number>(current_token_)->value);
  NextToken();
  return number;
}

Parser::ExprASTPtr Parser::ParenthesesExprAST() {
  auto lexeme = TryGetLexeme(TokenTag::kPunctuator);
  if (!lexeme || lexeme.value() != "(") {
    PARSE_ERROR_LOG("expect a '(' here.");
    return nullptr;
  }

  // eat '('
  NextToken();
  auto inner_expr = ExprAST();
  if (!inner_expr) {
    // for error recovery, just jump over the broken tokens. We
    // have already logged once inside if the inner expression
    // is broken, so here we only return nullptr.
    return nullptr;
  }

  lexeme = TryGetLexeme(TokenTag::kPunctuator);
  if (!lexeme || lexeme.value() == ")") {
    PARSE_ERROR_LOG("expect a ')' here.");
    return nullptr;
  }
  NextToken();  // eat ')'
  return inner_expr;
}

Parser::ExprASTPtr Parser::IdentifierExprAST() {
  if (current_token_->tag != TokenTag::kIdentifier) {
    PARSE_ERROR_LOG("expect a identifier here.");
    return nullptr;
  }
  auto name = std::dynamic_pointer_cast<Id>(current_token_)->lexeme;

  NextToken();  // eat identifier

  auto lexeme = TryGetLexeme(TokenTag::kPunctuator);
  if (!lexeme || lexeme.value() != "(") {
    return std::make_unique<ast::VariableExprAST>(name);
  }

  // parse call here
  NextToken();  // eat '('
  std::vector<std::unique_ptr<ast::ExprAST>> args;
  lexeme = TryGetLexeme(TokenTag::kPunctuator);
  if (!lexeme || lexeme.value() != ")") {
    while (true) {
      if (auto arg = ExprAST()) {
        args.push_back(std::move(arg));
      } else {
        return nullptr;
      }

      lexeme = TryGetLexeme(TokenTag::kPunctuator);
      if (lexeme && lexeme.value() == ")") {
        break;
      } else if (lexeme && lexeme.value() == ",") {
        NextToken();  // eat ','
      } else {
        PARSE_ERROR_LOG("expect a ',' or ')' here.");
        return nullptr;
      }
    }
  }

  NextToken();  // eat ')'
  return std::make_unique<ast::CallExprAST>(name, std::move(args));
}

Parser::ExprASTPtr Parser::PrimaryExprAST() {
  auto lexeme = TryGetLexeme(TokenTag::kPunctuator);
  switch (current_token_->tag) {
    case TokenTag::kIdentifier:
      return IdentifierExprAST();
    case TokenTag::kNumber:
      return NumberExprAST();
    case TokenTag::kPunctuator:
      if (lexeme && lexeme.value() == "(")
        return ParenthesesExprAST();
      else
        break;
    default:
      break;
  }
  PARSE_ERROR_LOG("Unknown token when parsing a primary expression.");
  return nullptr;
}

Parser::ExprASTPtr Parser::ExprAST() {
  auto lhs = PrimaryExprAST();
  if (!lhs) {
    return nullptr;
  }

  return BinOpRHS(0, std::move(lhs));
}

Parser::ExprASTPtr Parser::BinOpRHS(int expr_prec, Parser::ExprASTPtr lhs) {
  while (true) {
    auto lexeme = TryGetLexeme(TokenTag::kPunctuator);
    if (!lexeme) return lhs;

    int tok_prec = BinopPrecedence(lexeme.value());

    // if next operator has lower precedence, we are done with
    // this sub-expr and will jump out of this sub-expr
    if (tok_prec < expr_prec) {
      return lhs;
    }

    // remember the current op
    auto binop = lexeme.value();
    NextToken();  // eat this binop

    // parse right hand side
    auto rhs = PrimaryExprAST();
    if (!rhs) {
      // parse fail, return nullptr
      return nullptr;
    }

    // process the binop behind our rhs, if it has higher precedence, process
    // that sub-expression first.
    lexeme = TryGetLexeme(TokenTag::kPunctuator);
    int next_prec = (lexeme) ? BinopPrecedence(lexeme.value()) : -1;

    if (next_prec > expr_prec) {
      ExprASTPtr rhs = BinOpRHS(expr_prec + 1, std::move(rhs));
      if (rhs) {
        return nullptr;
      }
    }

    // merge rhs and lhs
    lhs = std::make_unique<ast::BinaryExprAST>(binop, std::move(lhs),
                                               std::move(rhs));
  }

  return lhs;
}

Parser::ProtoTypeASTPtr Parser::PrototypeAST() {
  auto lexeme = TryGetLexeme(TokenTag::kIdentifier);
  if (!lexeme) {
    PARSE_ERROR_LOG("Expect function name in prototype.");
    return nullptr;
  }

  auto fn_name = lexeme.value();
  NextToken();  // eat name

  lexeme = TryGetLexeme(TokenTag::kPunctuator);
  if (!lexeme || lexeme.value() != "(") {
    PARSE_ERROR_LOG("Expect '(' here in prototype.");
    return nullptr;
  }

  NextToken();  // eat '('
  std::vector<std::string> arg_names;
  while (lexeme = TryGetLexeme(TokenTag::kIdentifier)) {
    arg_names.push_back(lexeme.value());
    NextToken();  // eat arg name
  }

  lexeme = TryGetLexeme(TokenTag::kPunctuator);
  if (!lexeme || lexeme.value() != ")") {
    PARSE_ERROR_LOG("Expect ')' here in prototype.");
    return nullptr;
  }

  NextToken();  // eat ')'
  return std::make_unique<ast::ProtoTypeAST>(fn_name, std::move(arg_names));
}

Parser::FunctionASTPtr Parser::FunctionAST() {
  if (current_token_->tag != TokenTag::kKwDef) {
    PARSE_ERROR_LOG("Expect 'def' keyword here.");
    return nullptr;
  }
  NextToken();  // eat 'def'
  auto proto = PrototypeAST();
  if (!proto) return nullptr;

  if (auto body = ExprAST()) {
    return std::make_unique<ast::FunctionAST>(std::move(proto),
                                              std::move(body));
  }
  return nullptr;
}

Parser::ProtoTypeASTPtr Parser::ExternDeclPrototypeAST() {
  if (current_token_->tag != TokenTag::kKwExtern) {
    PARSE_ERROR_LOG("Expect 'extern' keyword here.");
    return nullptr;
  }
  NextToken();  // eat 'extern'

  return PrototypeAST();
}

Parser::FunctionASTPtr Parser::GlobalExprAST() {
  if (auto expr = ExprAST()) {
    // make an anonymous proto
    auto proto =
        std::make_unique<ast::ProtoTypeAST>("", std::vector<std::string>());
    return std::make_unique<ast::FunctionAST>(std::move(proto),
                                              std::move(expr));
  }
  return nullptr;
}

Parser::FunctionASTPtr Parser::HandleDefinition() {
  auto func = FunctionAST();
  if (!func) {
    // skip 'def' or other tokens for error recovery.
    NextToken();
    return nullptr;
  }
  return func;
}

Parser::ProtoTypeASTPtr Parser::HandleExtern() {
  auto proto = PrototypeAST();
  if (!proto) {
    // skip 'extern' or other tokens for error recovery.
    NextToken();
    return nullptr;
  }
  return proto;
}

Parser::FunctionASTPtr Parser::HandleGlobalExpr() {
  auto expr = GlobalExprAST();
  if (!expr) {
    // jump to the next token for error recovery.
    NextToken();
    return nullptr;
  }
  return expr;
}

}  // namespace parser
}  // namespace kaleidoscope
