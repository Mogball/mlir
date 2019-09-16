#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/Function.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/OpImplementation.h"
#include "mlir/IR/TypeSupport.h"
#include "mlir/IR/Types.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Module.h"
#include "mlir/IR/StandardTypes.h"
#include "mlir/Analysis/Verifier.h"

#include <memory>
#include <vector>

#include <iostream>

using namespace mlir;

namespace meow {
#define GET_OP_CLASSES
#include "toy/MeowOps.h.inc"
}

using std::cout;
using std::endl;

namespace meow {

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

  std::string &getVal() { return value; }
  Location &getLoc() { return location; }

private:
  Location location;
  std::string value;
};

class MeowModuleAST {
public:
  explicit MeowModuleAST(Location loc, std::vector<MeowExprAST> exprs)
    : meowExprs(std::move(exprs)) {}

  std::vector<MeowExprAST> &getExprs() { return meowExprs; }

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
        //cout << lexer.getString() << endl;
        meows.emplace_back(lexer.getLastLocation(), lexer.getString());
        lexer.getNextToken();
        break;
      case tok_eof:
        break;
      case tok_string:
        //cout << "xd" << endl;
        lexer.getNextToken();
        break;
      }
    }
    return std::make_unique<MeowModuleAST>(startLocation, std::move(meows));
  }

private:
  MeowLexerImpl lexer;
};

class MeowDialect : public mlir::Dialect {
public:
  explicit MeowDialect(mlir::MLIRContext *ctx)
    : mlir::Dialect("meow", ctx) {
    addOperations<
#define GET_OP_LIST
#include "toy/MeowOps.cpp.inc"
      >();
  }

  static llvm::StringRef getDialectNamespace() { return "meow"; }
};

class MeowGen {
public:
  explicit MeowGen(mlir::MLIRContext &context) : builder(&context) {}

  mlir::ModuleOp mlirGen(MeowModuleAST &ast) {
    auto module = mlir::ModuleOp::create(builder.getUnknownLoc());
    llvm::SmallVector<mlir::Type, 4> arg_types(0, builder.getF64Type());
    auto toplevel_func_type = builder.getFunctionType(arg_types, llvm::None);
    auto toplevel_func = mlir::FuncOp::create(builder.getUnknownLoc(), "toplevel", toplevel_func_type);
    auto &entryBlock = *toplevel_func.addEntryBlock();

    builder.setInsertionPointToStart(&entryBlock);

    for (auto &meowExpr : ast.getExprs()) {
      auto l = meowExpr.getLoc();
      builder.create<MeowOp>(builder.getFileLineColLoc(builder.getIdentifier(*l.file), l.line, l.column),
          StringAttr::get(meowExpr.getVal(), builder.getContext()));
    }

    builder.create<RetOp>(builder.getUnknownLoc());


    module.push_back(toplevel_func);


    if (failed(mlir::verify(module))) {
      module.emitError("Module verification errors");
      return nullptr;
    }

    return module;
  }

private:
  mlir::OpBuilder builder;
};

void doThang(llvm::StringRef buf) {
  using std::cout;
  using std::endl;
  MeowLexerImpl lexer(buf.begin(), buf.end(), "meow.meow");
  MeowParser parser(lexer);
  auto mast = parser.ParseModule();
  mlir::MLIRContext ctx;
  MeowGen gen(ctx);
  mlir::OwningModuleRef ret = gen.mlirGen(*mast);
  if (ret)
    ret->dump();
}


}

using namespace mlir;
using namespace meow;

#define GET_OP_CLASSES
#include "toy/MeowOps.cpp.inc"
