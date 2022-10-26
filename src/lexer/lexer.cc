#include "kaleidoscope/lexer.h"

#include <optional>
#include <type_traits>
#include <unordered_map>

#include "file.h"

namespace kaleidoscope {
namespace lexer {

namespace {

//////////////////////// Utils ////////////////////////
/*!
 * \brief Char utils
 */
struct CharUtils {
  static bool IsDigit(char c) { return c >= '0' && c <= '9'; }

  static bool IsLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
  }

  static bool IsOctalDigit(char c) { return c >= '0' && c <= '7'; }

  static bool IsBinaryDigit(char c) { return c == '0' || c == '1'; }

  static bool IsHexDigit(char c) {
    return IsDigit(c) || (c >= 'a' && c <= 'f');
  }

  static bool IsBlank(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
  }

  static bool IsValidIdElem(char c) {
    return IsDigit(c) || IsLetter(c) || c == '_';
  }

  static bool IsExpChar(char c) { return c == 'e' || c == 'E'; }
};

std::optional<char> ReadEscapeChar(char escape_last) {
  // we do not support numeric escape / universal-char escape here
  // the question mark \? is also not supported.
  switch (escape_last) {
    case 'n':
      return '\n';
    case 'a':
      return '\a';
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    case 'v':
      return '\v';
    case '"':
      return '\"';
    case '\'':
      return '\'';
    case '\\':
      return '\\';
    case '0':
      return '\0';
    default:
      break;
  }
  return std::nullopt;
}

enum class NumberState {
  kStart = 0,
  kAfterSign,  // first elem before sign
  kDecimalDigit,
  kDecimalEnd,
  kNotDecimalIntStart,
  kFirstHexDigit,
  kHexDigit,
  kHexEnd,
  kOctalDigit,
  kOctalEnd,
  kFirstBinaryDigit,
  kBinaryDitit,
  kBinaryEnd,
  kIntZero,
  kAfterDotDigit,
  kAfterDotDigitOpt,
  kFractionDigit,
  kAfterExpChar,
  kAfterExpSign,
  kExpDigit,
  kFloatEnd
};

}  // namespace

//////////////////////// Lexer Impl Class ////////////////////////
/*!
 * \brief Implement of Lexer
 */
using TokenPtr = std::shared_ptr<Token>;

class Lexer::Impl {
 public:
  using WordTable = std::unordered_map<std::string, std::shared_ptr<Word>>;

  Impl() = delete;

  Impl(const std::string& src_path)
      : file_(src_path), location_(SourceLocation::Begin()) {
    RegisterKeyWords();
    peek_ = file_.Peek();
  }

  /*! \brief Reset Lexer's state */
  void Reset();

  /*! \brief Reset this Lexer to another file */
  void ResetFile(const std::string& src_path);

  /*! \brief Get the next token */
  TokenPtr NextToken();

  /*! \brief Have we finish reading this source file */
  bool IsFinish() const;

 private:
  /*!
   * \brief Skip a single-line comment
   *
   * A single-line comment start with a '#'
   */
  void SkipSingleLineComment();

  // void SkipMultiLineComment();

  /*! \brief Skip all blank characters before next token */
  void SkipBlank();

  /*! \brief Get a double precision floating number token */
  TokenPtr GetNumber();

  // TokenPtr GetStringLiteral();

  /*! \brief Get Identifier or keywords */
  TokenPtr GetIdAndWord();

  // TokenPtr GetConstChar();

  /*! \brief Get symbols */
  TokenPtr GetPunctuator();

  template <typename T, std::enable_if_t<std::is_base_of_v<Word, T>>* = nullptr>
  void ReserveWord(T&& word) {
    auto lexeme = word.lexeme;
    bool no_duplicate =
        word_table_.insert({lexeme, std::make_shared<T>(std::forward<T>(word))})
            .second;
    if (!no_duplicate) {
      LOG_WARNING << "Find duplicated reserved word \"" << lexeme << "\""
                  << std::endl;
    }
  }

  void RegisterKeyWords();

  TokenPtr InsertId(Id id);

 private:
  WordTable word_table_;
  SourceLocation location_;
  char peek_;
  SourceFile file_;
  char lexeme_buffer_[SourceFile::kBufferSize + 1];
};

void Lexer::Impl::Reset() {
  location_ = SourceLocation::Begin();
  word_table_.clear();
  this->RegisterKeyWords();
  file_.Reset();
  peek_ = file_.Peek();
  lexeme_buffer_[0] = SourceFile::kEOF;
}

void Lexer::Impl::ResetFile(const std::string& src_path) {
  file_.CloseAndOpenOther(src_path);
  this->Reset();
}

TokenPtr Lexer::Impl::NextToken() {
  while (true) {
    SkipBlank();
    if (file_.StartWith("#")) {
      SkipSingleLineComment();
      continue;
    }
    location_ = file_.GetStartLocation();

    // number
    auto number_token_ptr = GetNumber();
    if (number_token_ptr) {
      number_token_ptr->location = location_;
      return number_token_ptr;
    }

    // keyword and id
    auto word_token_ptr = GetIdAndWord();
    if (word_token_ptr) {
      word_token_ptr->location = location_;
      return word_token_ptr;
    }

    // punctuator
    auto punctuator_token_ptr = GetPunctuator();
    if (punctuator_token_ptr) {
      punctuator_token_ptr->location = location_;
      return punctuator_token_ptr;
    }

    // eof
    if (file_.Peek() == SourceFile::kEOF) {
      auto ptr = std::make_shared<EOFToken>();
      ptr->location = location_;
      return ptr;
    }

    break;
  }

  LOG_ERROR << "[Lex Error]: Unknown token";
  return nullptr;
}

bool Lexer::Impl::IsFinish() const { return !file_.HasNext(); }

void Lexer::Impl::SkipBlank() {
  peek_ = file_.Peek();
  while (CharUtils::IsBlank(peek_)) {
    file_.ScanChar();
    file_.NextLexeme();
    peek_ = file_.Peek();
  }
}

void Lexer::Impl::SkipSingleLineComment() {
  assert(file_.StartWith("#"));
  do {
    peek_ = file_.ScanChar();
    file_.NextLexeme();
  } while (peek_ != '\n' && peek_ != SourceFile::kEOF);
}

TokenPtr Lexer::Impl::InsertId(Id id) {
  auto entry = word_table_.find(id.lexeme);
  if (entry == word_table_.end()) {
    word_table_[id.lexeme] = std::make_shared<Id>(Id(std::move(id)));
  }
  return word_table_[id.lexeme];
}

TokenPtr Lexer::Impl::GetIdAndWord() {
  int buffer_idx = 0;

  if (!CharUtils::IsValidIdElem(file_.Peek())) {
    return nullptr;
  }

  while (true) {
    peek_ = file_.Peek();
    if (CharUtils::IsValidIdElem(peek_)) {
      lexeme_buffer_[buffer_idx++] = peek_;
      file_.ScanChar();
    } else {
      break;
    }
  }
  lexeme_buffer_[buffer_idx] = SourceFile::kEOF;
  file_.NextLexeme();

  return InsertId(Id(lexeme_buffer_));
}

// currently we only support single-char punctuator
TokenPtr Lexer::Impl::GetPunctuator() {
  static const char* multi_punct[] = {
      "<<=", ">>=", "==", "!=", "<=", ">=", "->", "+=", "-=", "*=", "/=",
      "++",  "--",  "%=", "&=", "|=", "^=", "&&", "||", "<<", ">>",
  };

  for (auto mpunct : multi_punct) {
    if (file_.StartWith(mpunct)) {
      file_.Eat(static_cast<int>(std::strlen(mpunct)));
      return std::make_shared<Punctuator>(mpunct);
    }
  }

  char spunct = file_.Peek();
  if (std::ispunct(spunct)) {
    file_.ScanChar();
    file_.NextLexeme();
    std::string key(1, spunct);
    return std::make_shared<Punctuator>(key);
  } else
    return nullptr;
}

TokenPtr Lexer::Impl::GetNumber() {
  NumberState state = NumberState::kStart;
  bool scan_failed = false;
  int buffer_idx = 0;
  std::shared_ptr<Number> ret;

  while (!scan_failed) {
    peek_ = file_.Peek();
    if (peek_ == SourceFile::kEOF) {
      scan_failed = true;
      break;
    }
    if (buffer_idx >= SourceFile::kBufferSize) {
      LOG_ERROR << "[Lex Error]: int const too long";
    }
    switch (state) {
      case NumberState::kStart:
        if (peek_ == '+' || peek_ == '-') {
          // NOTE: sign should be treated as a unary op
          scan_failed = true;
          break;
          // lexeme_buffer_[buffer_idx++] = peek_;
          // file_.ScanChar();
        }
        state = NumberState::kAfterSign;
        break;

      case NumberState::kAfterSign:
        if (CharUtils::IsDigit(peek_)) {
          lexeme_buffer_[buffer_idx++] = peek_;
          state = (peek_ == '0') ? NumberState::kNotDecimalIntStart
                                 : NumberState::kDecimalDigit;
          file_.ScanChar();
        } else if (peek_ == '.') {
          lexeme_buffer_[buffer_idx++] = peek_;
          state = NumberState::kAfterDotDigit;
          file_.ScanChar();
        } else {
          scan_failed = true;
        }
        break;

      case NumberState::kDecimalDigit:
        if (CharUtils::IsDigit(peek_)) {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
        } else if (peek_ == '.') {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
          state = NumberState::kAfterDotDigitOpt;
        } else if (CharUtils::IsExpChar(peek_)) {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
          state = NumberState::kAfterExpChar;
        } else {
          state = NumberState::kDecimalEnd;
        }
        break;

      case NumberState::kDecimalEnd:
        scan_failed = true;
        lexeme_buffer_[buffer_idx] = SourceFile::kEOF;
        file_.NextLexeme();
        return std::make_shared<Number>(
            static_cast<double>(std::stoll(lexeme_buffer_)));

      case NumberState::kNotDecimalIntStart:
        if (peek_ == 'x') {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
          state = NumberState::kFirstHexDigit;
        } else if (peek_ == 'b') {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
          state = NumberState::kFirstBinaryDigit;
        } else if (CharUtils::IsOctalDigit(peek_)) {
          state = NumberState::kOctalDigit;
        } else if (peek_ == '.') {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
          state = NumberState::kAfterDotDigitOpt;
        } else {
          state = NumberState::kIntZero;
        }
        break;

      case NumberState::kFirstHexDigit:
        if (CharUtils::IsHexDigit(peek_)) {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
          state = NumberState::kHexDigit;
        } else {
          LOG_ERROR << "[Lex Error]: Invalid char " << peek_
                    << " in hex int const";
        }
        break;

      case NumberState::kHexDigit:
        if (CharUtils::IsHexDigit(peek_)) {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
          state = NumberState::kHexDigit;
        } else {
          state = NumberState::kHexEnd;
        }
        break;

      case NumberState::kHexEnd:
        lexeme_buffer_[buffer_idx] = SourceFile::kEOF;
        file_.NextLexeme();
        return std::make_shared<Number>(
            static_cast<double>(std::stoll(lexeme_buffer_, nullptr, 16)));

      case NumberState::kOctalDigit:
        if (CharUtils::IsOctalDigit(peek_)) {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
          state = NumberState::kOctalDigit;
        } else {
          state = NumberState::kOctalEnd;
        }
        break;

      case NumberState::kOctalEnd:
        lexeme_buffer_[buffer_idx] = SourceFile::kEOF;
        file_.NextLexeme();
        return std::make_shared<Number>(
            static_cast<double>(std::stoll(lexeme_buffer_, nullptr, 8)));

      case NumberState::kFirstBinaryDigit:
        if (CharUtils::IsBinaryDigit(peek_)) {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
          state = NumberState::kBinaryDitit;
        } else {
          LOG_ERROR << "[Lex Error]: Invalid char " << peek_
                    << " in binary int const";
        }
        break;

      case NumberState::kBinaryDitit:
        if (CharUtils::IsBinaryDigit(peek_)) {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
          state = NumberState::kBinaryDitit;
        } else {
          state = NumberState::kBinaryEnd;
        }
        break;

      case NumberState::kBinaryEnd:
        lexeme_buffer_[buffer_idx] = SourceFile::kEOF;
        file_.NextLexeme();
        return std::make_shared<Number>(
            static_cast<double>(std::stoll(lexeme_buffer_ + 2, nullptr, 2)));

      case NumberState::kIntZero:
        file_.NextLexeme();
        return std::make_shared<Number>(0);

      case NumberState::kAfterDotDigit:
        if (CharUtils::IsDigit(peek_)) {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
          state = NumberState::kFractionDigit;
        } else {
          scan_failed = true;
        }
        break;

      case NumberState::kAfterDotDigitOpt:
        if (CharUtils::IsDigit(peek_)) {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
          state = NumberState::kFractionDigit;
        } else if (CharUtils::IsExpChar(peek_)) {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
          state = NumberState::kAfterExpChar;
        } else {
          state = NumberState::kFloatEnd;
        }
        break;

      case NumberState::kFractionDigit:
        if (CharUtils::IsDigit(peek_)) {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
        } else if (CharUtils::IsExpChar(peek_)) {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
          state = NumberState::kAfterExpChar;
        } else {
          state = NumberState::kFloatEnd;
        }
        break;

      case NumberState::kAfterExpChar:
        if (peek_ == '-' || peek_ == '+') {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
        }
        state = NumberState::kAfterExpSign;
        break;

      case NumberState::kAfterExpSign:
        if (CharUtils::IsDigit(peek_)) {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
          state = NumberState::kExpDigit;
        } else {
          LOG_ERROR << "[Lex Error]: No digit is found after E/e";
        }
        break;

      case NumberState::kExpDigit:
        if (CharUtils::IsDigit(peek_)) {
          lexeme_buffer_[buffer_idx++] = peek_;
          file_.ScanChar();
        } else {
          state = NumberState::kFloatEnd;
        }
        break;

      case NumberState::kFloatEnd:
        lexeme_buffer_[buffer_idx] = SourceFile::kEOF;
        file_.NextLexeme();
        return std::make_shared<Number>(std::stod(lexeme_buffer_));

      default:
        LOG_ERROR << "[Lex Error]: Unknown int state";
    }
  }

  file_.ResetForward();
  return nullptr;
}

// void Lexer::Impl::SkipMultiLineComment() {
//   assert(file_.StartWith("/*"));
//   file_.Eat(2);
//   while (!file_.StartWith("*/")) {
//     peek_ = file_.ScanChar();
//     file_.NextLexeme();
//     CHECK(peek_ != SourceFile::kEOF) << "[Lex Error]: unfinished multi line
//     comment";
//   }
//   file_.Eat(2);
// }

// Lexer::Impl::TokenPtr Lexer::Impl::GetStringLiteral() {
//   bool escape = false;
//   int buffer_idx = 0;
//   peek_ = file_.ScanChar();
//   if (peek_ != '\"') {
//     file_.ResetForward();
//     return nullptr;
//   }
//   while (true) {
//     peek_ = file_.ScanChar();
//     CHECK(buffer_idx < SourceFile::kBufferSize) << "[Lex Error]: string
//     literal too long"; if (escape) {
//       auto read_ret = ReadEscapeChar(peek_);
//       escape = false;
//       CHECK(read_ret.has_value()) << "[Lex Error]: Unknown/unsupport escape
//       char"; peek_ = read_ret.value();
//     } else if (peek_ == '\\') {
//       escape = true;
//       continue;
//     } else if (peek_ == '\"') {
//       lexeme_buffer_[buffer_idx] = SourceFile::kEOF;
//       file_.NextLexeme();
//       return std::make_shared<StringLiteral>(lexeme_buffer_);
//     } else if (peek_ == SourceFile::kEOF || peek_ == '\n') {
//       LOG_ERROR << "[Lex error]: Unfinished string literal";
//     }
//     lexeme_buffer_[buffer_idx++] = peek_;
//   }
//   file_.ResetForward();
//   return nullptr;
// }

void Lexer::Impl::RegisterKeyWords() {
  ReserveWord(KwDef());
  ReserveWord(KwExtern());
}

//////////////////////// Lexer Functions ////////////////////////
/*!
 * \brief Construct a new Lexer:: Lexer object
 *
 */
Lexer::Lexer() = default;

Lexer::Lexer(const std::string& src_path)
    : pimpl_(std::make_unique<Impl>(src_path)) {}

Lexer::~Lexer() = default;

Lexer::Lexer(Lexer&& other) : pimpl_(other.pimpl_.release()) {}

Lexer& Lexer::operator=(Lexer&& other) {
  // move and swap
  using std::swap;
  Lexer tmp(std::move(other));
  swap(tmp.pimpl_, pimpl_);
  return *this;
}

bool Lexer::IsFinish() const { return pimpl_->IsFinish(); }

TokenPtr Lexer::NextToken() { return pimpl_->NextToken(); }

void Lexer::Reset() { pimpl_->Reset(); }

void Lexer::ResetFile(const std::string& path) { pimpl_->ResetFile(path); }

std::ostream& operator<<(std::ostream& sm, Lexer& lexer) {
  TokenPtr token = nullptr;
  do {
    token = lexer.NextToken();
    sm << *token << std::endl;
  } while (token->tag != TokenTag::kEOF);
  lexer.Reset();
  return sm;
}

}  // namespace lexer
}  // namespace kaleidoscope
