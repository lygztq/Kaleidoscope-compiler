/*!
 * \file file.h
 * \brief Source file abstration used by Lexer. This is a inner class
 */
#ifndef KALEIDOSCOPE_LEXER_FILE_H_
#define KALEIDOSCOPE_LEXER_FILE_H_

#include <array>
#include <fstream>
#include <string>

#include "kaleidoscope/logging.h"
#include "kaleidoscope/source_location.h"

namespace kaleidoscope {
namespace lexer {

class SourceFile {
 public:
  static constexpr size_t kBufferSize = 4096;
  static constexpr char kEOF = '\0';
  static constexpr char kNewLine = '\n';

 private:
  using stream_t = std::ifstream;
  using buffer_pair_t = std::array<std::array<char, kBufferSize>, 2>;
  using Location = SourceLocation;

  std::string path_;                // source file path
  stream_t stream_;                 // fstream used by this abstraction
  buffer_pair_t input_buffer_;      // input buffer pair
  int forward_;                     // current char scanned
  int begin_;                       // where current lexeme begin
  int extent_;                      // current lexeme extent
  int forward_buffer_idx_;          // buffer where forward position in
  int start_buffer_idx_;            // buffer where lexeme start position in
  Location forward_location_;       // forward location in source file
  Location start_location_;         // lexeme start location in source file
  int buff_timestamp_[2] = {0, 0};  // time stamp of buffers
  bool is_end_;                     // reach the end of the source file

 private:
  /*! \brief check whether stamp \a next is the next stemp of stamp \a origin */
  static bool IsNextStamp(int next, int origin) {
    return NextStamp(origin) == next;
  }

  /*! \brief get the next time stamp */
  static int NextStamp(int stamp) { return (stamp + 1) % 3; }

  /*! \brief move \a forward_ to the next valid buffer position */
  void NextValidPos();

 public:
  SourceFile() = delete;
  SourceFile(const std::string& path);
  friend void swap(SourceFile& f1, SourceFile& f2);
  SourceFile(SourceFile&& f);
  SourceFile& operator=(SourceFile&& f);

  // copy is not allowed
  SourceFile(const SourceFile&) = delete;
  SourceFile& operator=(const SourceFile&) = delete;

  ~SourceFile() {
    if (stream_.is_open()) stream_.close();
  }

  /*!
   * \brief Get forward location in the source file
   * \return scompiler::SourceLocation Location
   */
  Location GetForwardLocation() const { return forward_location_; }

  /*!
   * \brief Get lexeme start location in the source file
   * \return scompiler::SourceLocation Location
   */
  Location GetStartLocation() const { return start_location_; }

  /*! \brief Load contains from disk to the current buffer. */
  void LoadBuffer();

  /*! \brief Change to another input buffer */
  void ChangeForwardBuffer() { forward_buffer_idx_ = 1 ^ forward_buffer_idx_; }

  /*! \brief Check if has next block to be loaded */
  bool HasNext() const { return !is_end_; }

  /*! \brief Get index of the current buffer */
  int CurrentForwardBufferIndex() const { return forward_buffer_idx_; }

  /*! \brief Get the current buffer */
  const char* CurrentForwardBuffer() const {
    return input_buffer_[forward_buffer_idx_].data();
  }

  /*! \brief Check if we reach at the end of the current buffer */
  bool IsForwardBufferEnd() const { return forward_ >= (kBufferSize - 1); }

  /*! \brief Get char pointed by forward_ and move forward_ ptr to the next */
  char ScanChar();

  /*! \brief Change to a new lexeme */
  void NextLexeme() {
    begin_ = forward_;
    extent_ = 0;
    start_buffer_idx_ = forward_buffer_idx_;
    start_location_ = forward_location_;
  }

  /*! \brief Reset forward to lexeme start */
  void ResetForward() {
    forward_ = begin_;
    extent_ = 0;
    forward_buffer_idx_ = start_buffer_idx_;
    forward_location_ = start_location_;
  }

  /*!
   * \brief Check whether the current lexeme in buffer is start with
   *        the given pattern
   */
  bool StartWith(const std::string& pattern);

  /*!
   * \brief Eat \a num_char characters without processing them
   *
   * \param num_char The number of characters to be eaten.
   */
  void Eat(int num_char);

  /*! \brief Show the peek char */
  char Peek() { return CurrentForwardBuffer()[forward_]; }

  /*! \brief Reset the file state */
  void Reset();

  void CloseAndOpenOther(const std::string& path);
};

}  // namespace lexer
}  // namespace kaleidoscope

#endif  // KALEIDOSCOPE_LEXER_FILE_H_
