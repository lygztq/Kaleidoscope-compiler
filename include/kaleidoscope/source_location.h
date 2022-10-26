#ifndef KALEIDOSCOPE_SOURCE_LOCATION_H_
#define KALEIDOSCOPE_SOURCE_LOCATION_H_

#include <sstream>
#include <string>

#include "kaleidoscope/macro.h"

namespace kaleidoscope {

/*! \brief Location in a source file */
struct SourceLocation {
  int line;  // line index in the source file
  int col;   // column index in the source file
  int pos;   // absolute offset from the beginning of the source file

  SourceLocation() = default;
  explicit SourceLocation(int line, int col, int pos)
      : col(col), line(line), pos(pos) {}
  static SourceLocation Begin() { return SourceLocation(0, 0, 0); }
  SourceLocation(const SourceLocation&) = default;
  SourceLocation(SourceLocation&&) = default;
  SourceLocation& operator=(const SourceLocation&) = default;
  SourceLocation& operator=(SourceLocation&&) = default;

  /*!
   * \brief dump this source code location as a string
   * \return std::string
   */
  std::string Dump() const {
    std::stringstream sm;
    sm << "Line: " << line << ", "
       << "Col: " << col << ", "
       << "Offset: " << pos;
    return sm.str();
  }

  void AdvanceCol() {
    ++col;
    ++pos;
  }

  void AdvanceLine() {
    ++line;
    ++pos;
  }

  bool operator==(const SourceLocation& other) const {
    return line == other.line && col == other.col && pos == other.pos;
  }

  operator std::string() const { return this->Dump(); }
};

/*!
 * \brief out stream of SourceLocation
 * \param os
 * \param loc
 * \return std::ostream&
 */
inline std::ostream& operator<<(std::ostream& out, const SourceLocation& loc) {
  out << loc.Dump();
  return out;
}

struct SourceLocationRange {
  SourceLocation begin;
  SourceLocation end;
};

}  // namespace kaleidoscope

#endif  // KALEIDOSCOPE_SOURCE_LOCATION_H_
