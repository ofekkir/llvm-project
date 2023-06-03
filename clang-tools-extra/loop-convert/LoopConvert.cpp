// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "llvm/Support/CommandLine.h"

using namespace clang::tooling;
using namespace llvm;
using namespace clang;
using namespace clang::ast_matchers;

auto LoopMatcher =
  recordDecl(
    hasDescendant(
      fieldDecl(
        hasType(
          pointsTo(
            recordDecl(
              hasDescendant(
                fieldDecl(
                  hasType(
                    recordDecl(
                      hasName("mutex")
                    )
                  )
                ).bind("internal_member")
              )
            ).bind("internal_container")
          )
        )
      ).bind("external_pointer")
    )
  ).bind("external_container");

// auto LoopMatcher =
// pointerType(pointee(hasMemberName("koko"))).bind("pointer");
// auto LoopMatcher = fieldDecl(hasType(pointerType())).bind("pointer");

class LoopPrinter : public MatchFinder::MatchCallback {
public:
  virtual void run(const MatchFinder::MatchResult &Result) override {
    if (const auto *res =
            Result.Nodes.getNodeAs<clang::RecordDecl>("internal_container"))
      res->dump();
  }
};

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

int main(int argc, const char **argv) {
  auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser &OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  LoopPrinter Printer;
  MatchFinder Finder;
  Finder.addMatcher(LoopMatcher, &Printer);

  return Tool.run(newFrontendActionFactory(&Finder).get());
}
