#include <exception>
#include <filesystem>
#include <fstream>
#include <string>

#include "kaleidoscope/lexer.h"
#include "kaleidoscope/logging.h"

int main(int argc, char** argv) {
  namespace fs = std::filesystem;
  if (argc != 3) {
    throw std::runtime_error(
        "A source file path and a destination path should be given.");
  }

  std::string src_path(argv[1]);
  std::string dst_path(argv[2]);

  kaleidoscope::lexer::Lexer lexer(src_path);

  auto out_dir = fs::path(dst_path).parent_path();
  if (!fs::exists(out_dir)) {
    fs::create_directories(out_dir);
  }
  std::ofstream out_sm(dst_path);
  out_sm << lexer;

  return 0;
}
