#ifndef KALEIDOSCOPE_LEXER_H_
#define KALEIDOSCOPE_LEXER_H_

#include <memory>
#include <ostream>

#include "kaleidoscope/macro.h"
#include "kaleidoscope/token.h"

namespace kaleidoscope {
namespace lexer {

class Lexer {
 public:
  LEXER_DLL Lexer();
  LEXER_DLL Lexer(const std::string& src_path);
  LEXER_DLL ~Lexer();
  Lexer(const Lexer&) = delete;
  LEXER_DLL Lexer(Lexer&&);
  Lexer& operator=(const Lexer&) = delete;
  LEXER_DLL Lexer& operator=(Lexer&&);

  bool Empty() const { return pimpl_ == nullptr; }

  LEXER_DLL bool IsFinish() const;

  LEXER_DLL std::shared_ptr<Token> NextToken();

  LEXER_DLL void Reset();

  LEXER_DLL void ResetFile(const std::string& src_path);

  LEXER_DLL friend std::ostream& operator<<(std::ostream& sm, Lexer& lexer);

 private:
  class Impl;
  std::unique_ptr<Impl> pimpl_ = nullptr;
};

}  // namespace lexer
}  // namespace kaleidoscope

#endif  // KALEIDOSCOPE_LEXER_H_
