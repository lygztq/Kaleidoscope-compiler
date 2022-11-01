#ifndef KALEIDOSCOPE_PARSER_H_
#define KALEIDOSCOPE_PARSER_H_

#include <list>
#include <memory>
#include <optional>
#include <string>

#include "kaleidoscope/ast.h"
#include "kaleidoscope/lexer.h"
#include "kaleidoscope/macro.h"
#include "kaleidoscope/token.h"

namespace kaleidoscope {
namespace parser {

class Parser {
 public:
#define DEFPTR(type) using type##Ptr = std::unique_ptr<ast::type>;

  DEFPTR(AST);
  DEFPTR(ExprAST);
  DEFPTR(NumberExprAST);
  DEFPTR(VariableExprAST);
  DEFPTR(BinaryExprAST);
  DEFPTR(CallExprAST);
  DEFPTR(ProtoTypeAST);
  DEFPTR(FunctionAST);

#undef DEFPTR

  Parser() = delete;

  explicit Parser(const std::string& src_path)
      : lexer_(src_path), current_token_(nullptr) {}

  Parser(const Parser&) = delete;

  Parser(Parser&& other)
      : current_token_(std::move(other.current_token_)),
        lexer_(std::move(other.lexer_)) {}

  Parser& operator=(const Parser&) = delete;

  Parser& operator=(Parser&& other) {
    using std::swap;
    auto tmp = std::move(other);
    swap(tmp, other);
    return *this;
  }

  std::list<ASTPtr> Parse() {
    std::list<ASTPtr> ast_list;
    bool finish_parse = false;
    ASTPtr current = nullptr;
    std::optional<std::string> lexeme;
    bool has_error = false;
    lexer_.Reset();
    NextToken();

    while (!finish_parse) {
      switch (current_token_->tag) {
        case TokenTag::kEOF:
          finish_parse = true;
          break;
        case TokenTag::kKwDef:
          current = HandleDefinition();
          if (current) {
            ast_list.push_back(std::move(current));
          } else {
            has_error = true;
          }
          break;
        case TokenTag::kKwExtern:
          current = HandleExtern();
          if (current) {
            ast_list.push_back(std::move(current));
          } else {
            has_error = true;
          }
          break;
        case TokenTag::kPunctuator:
          lexeme = TryGetLexeme(TokenTag::kPunctuator);
          // ignore top-level semicolons
          if (lexeme && lexeme.value() == ";") {
            NextToken();
            break;
          }  // else goto default.
        default:
          current = HandleGlobalExpr();
          if (current) {
            ast_list.push_back(std::move(current));
          } else {
            has_error = true;
          }
          break;
      }
    }
    if (has_error) return {};
    return ast_list;
  }

 private:
  /*!
   * \brief Parse a NumberExpr
   *
   * \note numberexpr ::= number
   *
   * \return NumberExprASTPtr
   */
  PARSER_DLL NumberExprASTPtr NumberExprAST();

  /*!
   * \brief Parse a ParenthesesExpr
   *
   * \note parenexpr ::= '(' expr ')'
   *
   * \return ExprASTPtr
   */
  PARSER_DLL ExprASTPtr ParenthesesExprAST();

  /*!
   * \brief Parse a IdentifierExpr
   *
   * \note idexpr ::= identifier
   *              ::= identifier '(' expr* ')'
   *
   * \return ExprASTPtr
   */
  PARSER_DLL ExprASTPtr IdentifierExprAST();

  /*!
   * \brief Parse a PrimaryExpr
   *
   * \note primaryexpr ::= numberexpr
   *                   ::= parenexpr
   *                   ::= idexpr
   *
   * \return ExprASTPtr
   */
  PARSER_DLL ExprASTPtr PrimaryExprAST();

  /*!
   * \brief Parse a Expr
   *
   * \note expr ::= primaryexpr binoprhs
   *
   * \return ExprASTPtr
   */
  PARSER_DLL ExprASTPtr ExprAST();

  /*!
   * \brief Parse a right hand side of binary op expression
   *
   * \note binoprhs ::= ('+' primary)*
   *
   * \return ExprASTPtr
   */
  PARSER_DLL ExprASTPtr BinOpRHS(int precedence, ExprASTPtr lhs);

  /*!
   * \brief Parse a function's prototype
   *
   * \note prototype ::= id '(' id* ')'
   *
   * \return ProtoTypeASTPtr
   */
  PARSER_DLL ProtoTypeASTPtr PrototypeAST();

  /*!
   * \brief Parse a function
   *
   * \note function ::= 'def' prototype expr
   *
   * \return FunctionASTPtr
   */
  PARSER_DLL FunctionASTPtr FunctionAST();

  /*!
   * \brief Parse a external prototype declaration
   *
   * \note external ::= 'extern' prototype
   *
   * \return ProtoTypeASTPtr
   */
  PARSER_DLL ProtoTypeASTPtr ExternDeclPrototypeAST();

  /*!
   * \brief Parse a top-level expression
   *
   * \note topexpr ::= expr
   *
   * \return FunctionASTPtr
   */
  PARSER_DLL FunctionASTPtr GlobalExprAST();

  PARSER_DLL FunctionASTPtr HandleDefinition();

  PARSER_DLL ProtoTypeASTPtr HandleExtern();

  PARSER_DLL FunctionASTPtr HandleGlobalExpr();

  PARSER_DLL void NextToken();

  PARSER_DLL std::optional<std::string> TryGetLexeme(TokenTag tag) const;

 private:
  std::shared_ptr<Token> current_token_;
  mutable lexer::Lexer lexer_;
};

}  // namespace parser
}  // namespace kaleidoscope

#endif  // KALEIDOSCOPE_PARSER_H_
