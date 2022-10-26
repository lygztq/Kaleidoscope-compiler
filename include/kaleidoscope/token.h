#ifndef KALEIDOSCOPE_TOKEN_H_
#define KALEIDOSCOPE_TOKEN_H_

#include <mutex>
#include <sstream>
#include <string>

#include "kaleidoscope/source_location.h"
#include "nameof.hpp"

namespace kaleidoscope {

enum class TokenTag {
  // special
  kInvalid = 0,
  kEOF,

  // commands
  kDef,
  kExtern,

  // primary
  kIdentifier,
  kNumber,
  kPunctuator,

  // keywords
  kKwDef,
  kKwExtern,
};
constexpr int kNumTokenTag = 9;

void InitTokenNameTable(const char** tag_names) {
  tag_names[0] = "kInvalid";
  tag_names[1] = "kEOF";
  tag_names[2] = "kDef";
  tag_names[3] = "kExtern";
  tag_names[4] = "kIdentifier";
  tag_names[5] = "kNumber";
  tag_names[6] = "kPunctuator";
  tag_names[7] = "kKwDef";
  tag_names[8] = "kKwExtern";
}

const std::string_view DeprecateGetTokenTagName(TokenTag tag) {
  static const char* tag_names[kNumTokenTag];
  std::once_flag flag;
  std::call_once(flag, InitTokenNameTable, tag_names);

  return std::string_view(tag_names[static_cast<int>(tag)]);
}

const std::string_view GetTokenTagName(TokenTag tag) {
#if NAMEOF_TYPE_SUPPORTED
  return nameof::nameof_enum(tag);
#else
  return DeprecateGetTokenTagName(tag);
#endif
}

struct Token {
  TokenTag tag;
  SourceLocation location = SourceLocation::Begin();
  explicit Token(TokenTag tag) : tag(tag) {}
  virtual ~Token() {}
  virtual void Dump(std::ostream& sm) const {
    sm << "(Token: tag=" << GetTokenTagName(tag) << ")";
  }
  friend std::ostream& operator<<(std::ostream& sm, const Token& t) {
    t.Dump(sm);
    return sm;
  }
  SourceLocation GetLocation() const { return location; }
  void SetLocation(SourceLocation location) { this->location = location; }
};

template <typename T>
struct ConstValue : public Token {
  T value;
  explicit ConstValue(TokenTag tag, T val) : Token(tag), value(val) {}
  virtual void Dump(std::ostream& sm) const {
    sm << "(Value: tag=" << GetTokenTagName(tag) << ", value=" << value << ")";
  }
};

// A double-precision float number
struct Number : public ConstValue<double> {
  explicit Number(double v) : ConstValue(TokenTag::kNumber, v) {}
};

// struct StringLiteral : public ConstValue<std::string> {
//   explicit StringLiteral(const std::string& s)
//     : ConstValue(TokenTag::kStringLiteral, s) {}
//   virtual void Dump(std::ostream& sm) const {
//     sm << "(Value: tag=" << GetTokenTagName(tag) << ", value=\"" << value <<
//     "\")";
//   }
// };

// struct ConstChar : public ConstValue<char> {
//   explicit ConstChar(char c) : ConstValue(TokenTag::kConstChar, c) {}
// };

struct Word : public Token {
  std::string lexeme;
  explicit Word(TokenTag tag, const std::string& lexeme)
      : Token(tag), lexeme(lexeme) {}
  virtual void Dump(std::ostream& sm) const {
    sm << "(Word: tag=" << GetTokenTagName(tag) << ", lexeme=\"" << lexeme
       << "\")";
  }
};

struct Punctuator : public Word {
  explicit Punctuator(const std::string& lexeme)
      : Word(TokenTag::kPunctuator, lexeme) {}
};

struct Id : public Word {
  explicit Id(const std::string& lexeme)
      : Word(TokenTag::kIdentifier, lexeme) {}
};

#define DECL_KEYWORD(name, lexeme)                             \
  struct Kw##name : public Word {                              \
    explicit Kw##name() : Word(TokenTag::kKw##name, lexeme) {} \
  };

DECL_KEYWORD(Def, "def");
DECL_KEYWORD(Extern, "extern");

#undef DECL_KEYWORD

// #define DECL_PUNCTUATOR(name, punct)                    \
//   struct name : public Word {                           \
//     explicit name() : Word(TokenTag::k##name, punct) {} \
//   };

// DECL_PUNCTUATOR(Plus, "+");
// DECL_PUNCTUATOR(Minus, "-");

// #undef DECL_PUNCTUATOR

struct EOFToken : public Token {
  explicit EOFToken() : Token(TokenTag::kEOF) {}
};

}  // namespace kaleidoscope

#endif  // KALEIDOSCOPE_TOKEN_H_
