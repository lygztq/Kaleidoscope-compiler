#ifndef KALEIDOSCOPE_MACRO_H_
#define KALEIDOSCOPE_MACRO_H_

#ifndef KALEIDOSCOPE_DLL
#ifdef _WIN32
#ifdef KALEIDOSCOPE_EXPORT
#define KALEIDOSCOPE_DLL __declspec(dllexport)
#else  // KALEIDOSCOPE_EXPORT
#define KALEIDOSCOPE_DLL __declspec(dllimport)
#endif  // KALEIDOSCOPE_EXPORT
#else   // _WIN32
#define KALEIDOSCOPE_DLL
#endif  // _WIN32
#endif  // KALEIDOSCOPE_DLL

#ifndef LEXER_DLL
#ifdef _WIN32
#ifdef LEXER_EXPORT
#define LEXER_DLL __declspec(dllexport)
#else  // LEXER_EXPORT
#define LEXER_DLL __declspec(dllimport)
#endif  // LEXER_EXPORT
#else   // _WIN32
#define LEXER_DLL
#endif  // _WIN32
#endif  // LEXER_DLL

#ifndef PARSER_DLL
#ifdef _WIN32
#ifdef PARSER_EXPORT
#define PARSER_DLL __declspec(dllexport)
#else  // PARSER_EXPORT
#define PARSER_DLL __declspec(dllimport)
#endif  // PARSER_EXPORT
#else   // _WIN32
#define PARSER_DLL
#endif  // _WIN32
#endif  // PARSER_DLL

#endif  // KALEIDOSCOPE_MACRO_H_
