#include "file.h"

#include <utility>

namespace kaleidoscope {
namespace lexer {

SourceFile::SourceFile(const std::string& path)
    : path_(path),
      forward_(0),
      begin_(0),
      extent_(0),
      forward_buffer_idx_(0),
      start_buffer_idx_(0),
      forward_location_(Location::Begin()),
      start_location_(Location::Begin()),
      is_end_(false) {
#ifdef BUILD_DEBUG
  LOG_DEBUG << "Opening file: " << path << std::endl;
#endif
  stream_.open(path_, stream_.in);
  CHECK(stream_.is_open()) << "Cannot open file: " << path_;

  // init load
  LoadBuffer();
}

SourceFile::SourceFile(SourceFile&& f)
    : path_(f.path_),
      stream_(std::move(f.stream_)),
      forward_(f.forward_),
      begin_(f.begin_),
      extent_(f.extent_),
      forward_buffer_idx_(f.forward_buffer_idx_),
      start_buffer_idx_(f.start_buffer_idx_),
      forward_location_(f.forward_location_),
      start_location_(f.start_location_),
      is_end_(f.is_end_) {
  using std::swap;
  std::copy_n(f.buff_timestamp_, 2, buff_timestamp_);
  swap(input_buffer_, f.input_buffer_);
}

void swap(SourceFile& f1, SourceFile& f2) {
  using std::swap;
  swap(f1.path_, f2.path_);
  swap(f1.stream_, f2.stream_);
  swap(f1.input_buffer_, f2.input_buffer_);
  swap(f1.forward_, f2.forward_);
  swap(f1.begin_, f2.begin_);
  swap(f1.extent_, f2.extent_);
  swap(f1.forward_buffer_idx_, f2.forward_buffer_idx_);
  swap(f1.start_buffer_idx_, f2.start_buffer_idx_);
  swap(f1.forward_location_, f2.forward_location_);
  swap(f1.start_location_, f2.start_location_);
  swap(f1.buff_timestamp_, f2.buff_timestamp_);
  swap(f1.is_end_, f2.is_end_);
}

SourceFile& SourceFile::operator=(SourceFile&& f) {
  using std::swap;
  SourceFile tmp(std::move(f));
  swap(tmp, *this);
  return *this;
}

void SourceFile::LoadBuffer() {
  if (!IsNextStamp(buff_timestamp_[forward_buffer_idx_],
                   buff_timestamp_[start_buffer_idx_])) {
    stream_.read(input_buffer_[forward_buffer_idx_].data(), kBufferSize - 1);
    input_buffer_[forward_buffer_idx_][kBufferSize - 1] = kEOF;  // sentinel
    is_end_ = stream_.eof();
    if (is_end_) input_buffer_[forward_buffer_idx_][stream_.gcount()] = kEOF;
    buff_timestamp_[forward_buffer_idx_] =
        NextStamp(buff_timestamp_[start_buffer_idx_]);
  }
}

void SourceFile::NextValidPos() {
  char peek = Peek();
  if (peek == '\n') {
    forward_location_.AdvanceLine();
  } else if (peek == SourceFile::kEOF) {
    return;
  } else {
    forward_location_.AdvanceCol();
  }

  ++forward_;
  ++extent_;
  CHECK_LT(extent_, kBufferSize) << "Lexeme too long!";

  peek = Peek();
  if (peek == kEOF && HasNext() && IsForwardBufferEnd()) {
    forward_ = 0;
    ChangeForwardBuffer();
    LoadBuffer();
  }
}

char SourceFile::ScanChar() {
  char peek = Peek();
  NextValidPos();
  return peek;
}

bool SourceFile::StartWith(const std::string& pattern) {
  bool match = true;
  for (const auto& c : pattern) {
    if (ScanChar() != c) {
      match = false;
      break;
    }
  }
  ResetForward();
  return match;
}

void SourceFile::Eat(int num_char) {
  char curr;
  for (int i = 0; i < num_char; ++i) {
    curr = ScanChar();
    NextLexeme();
    if (curr == SourceFile::kEOF) break;
  }
}

void SourceFile::Reset() {
  stream_.seekg(0);
  forward_ = 0;
  begin_ = 0;
  extent_ = 0;
  forward_buffer_idx_ = 0;
  start_buffer_idx_ = 0;
  forward_location_ = Location::Begin();
  start_location_ = Location::Begin();
  is_end_ = false;
  LoadBuffer();
}

void SourceFile::CloseAndOpenOther(const std::string& path) {
  stream_.close();
  this->path_ = path;
  stream_.open(path, stream_.in);
  CHECK(stream_.is_open()) << "Cannot open file: " << path;
  this->Reset();
}

}  // namespace lexer
}  // namespace kaleidoscope
