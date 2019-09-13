#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <vector>

struct Location {
  std::shared_ptr<std::string> file;
  int line;
  int column;
};

enum Token : int {
  tok_meow,
  tok_string,
  tok_unknown,
  tok_eof
};

#include <iostream>

using std::cout;
using std::endl;

class MeowLexer {
public:
  MeowLexer(std::string filename)
    : lastLocation({std::make_shared<std::string>(std::move(filename)), 0, 0}) {}
  virtual ~MeowLexer() = default;

  Token getCurrentToken() { return currentToken; }
  Token getNextToken() { return currentToken = getToken(); }

  int getLine() { return currentLine; }
  int getColumn() { return currentColumn; }
  Location getLastLocation() { return lastLocation; }
  std::string getString() { return identifier; }

private:

  virtual llvm::StringRef readNextLine() = 0;

  int getNextCharacter() {
    if (currentLineBuffer.empty()) {
      return EOF;
    }
    ++currentColumn;
    auto nextCharacter = currentLineBuffer.front();
    currentLineBuffer = currentLineBuffer.drop_front();
    if (currentLineBuffer.empty()) {
      currentLineBuffer = readNextLine();
    }
    if (nextCharacter == '\n') {
      ++currentLine;
      currentColumn = 0;
    }
    return nextCharacter;
  }

  Token getToken() {
    lastCharacter = Token(getNextCharacter());

    while (isspace(lastCharacter)) {
      lastCharacter = Token(getNextCharacter());
    }

    if (EOF == lastCharacter) {
      return tok_eof;
    }


    lastLocation.line = currentLine;
    lastLocation.column = currentColumn;

    if (lastCharacter == '\"') {
      identifier = "";
      while ((lastCharacter = Token(getNextCharacter())) != '\"') {
        identifier += (char) lastCharacter;
      }
      return tok_string;
    }

    identifier = (char) lastCharacter;
    while (isalpha(lastCharacter = Token(getNextCharacter()))) {
      identifier += (char) lastCharacter;
    }

    if (identifier != "meow") {
      return tok_unknown;
    }
    return tok_meow;
  }

  Location lastLocation;
  int currentColumn;
  int currentLine;

  Token lastCharacter = Token(' ');
  Token currentToken;

  std::string identifier;

  llvm::StringRef currentLineBuffer = "\n";
};

class MeowLexerImpl final : public MeowLexer {
public:
  MeowLexerImpl(const char *begin, const char *end, std::string filename)
    : MeowLexer(std::move(filename)), current(begin), end(end) {}

private:
  llvm::StringRef readNextLine() override {
    auto *begin = current;
    while (current <= end && *current && *current != '\n') {
      ++current;
    }
    if (current <= end && *current) {
      ++current;
    }
    llvm::StringRef result{begin, static_cast<std::size_t>(current - begin)};
    return result;
  }

  const char *current;
  const char *end;
};

class MeowExprAST {
public:
  explicit MeowExprAST(Location loc, std::string value)
    : location(loc), value(value) {}

private:
  Location location;
  std::string value;
};

class MeowModuleAST {
public:
  explicit MeowModuleAST(Location loc, std::vector<MeowExprAST> exprs)
    : meowExprs(std::move(exprs)) {}

private:
  std::vector<MeowExprAST> meowExprs;
};

class MeowParser {
public:
  MeowParser(MeowLexerImpl &lexer) : lexer(lexer) {}

  std::unique_ptr<MeowModuleAST> ParseModule() {
    Location startLocation = lexer.getLastLocation();
    lexer.getNextToken();
    std::vector<MeowExprAST> meows;
    Token tok;
    while ((tok = lexer.getCurrentToken()) != tok_eof) {
      switch (tok) {
      case tok_meow:
        lexer.getNextToken();
        cout << lexer.getString() << endl;
        meows.emplace_back(lexer.getLastLocation(), lexer.getString());
        lexer.getNextToken();
        break;
      case tok_eof:
        break;
      case tok_string:
        cout << "xd" << endl;
        lexer.getNextToken();
        break;
      }
    }
    return std::make_unique<MeowModuleAST>(startLocation, std::move(meows));
  }

private:
  MeowLexerImpl lexer;
};

void doThang(llvm::StringRef buf) {
  using std::cout;
  using std::endl;
  MeowLexerImpl lexer(buf.begin(), buf.end(), "meow.meow");
  MeowParser parser(lexer);
  auto xd = parser.ParseModule();
}
