// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "llvm/Support/CommandLine.h"

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang::tooling;
using namespace llvm;
using namespace clang;
using namespace clang::ast_matchers;
namespace {


auto LoopMatcher =
  functionDecl(
    hasDescendant(
      callExpr(
        callee(functionDecl(hasName("mutex_lock"))),
        hasArgument(0, stmt(
          hasDescendant(
            memberExpr(
              hasDescendant(
                // Dumps from inside (mutex) out (container)
                memberExpr(
                  hasDescendant(
                    declRefExpr(

                    ).bind("external_decl_ref")
                  )
                ).bind("external_member")
              )
            ).bind("internal_member")
          )
        ).bind("stmt"))
      )
    )
  ).bind("function");
  



class LoopPrinter : public MatchFinder::MatchCallback {
public:
  virtual void run(const MatchFinder::MatchResult &Result) override {
    const auto *member = Result.Nodes.getNodeAs<clang::MemberExpr>("external_member");
    if (!member) return;

    const auto *decl_ref = Result.Nodes.getNodeAs<clang::DeclRefExpr>("external_decl_ref");
    if (!decl_ref) return;

    const auto *stmt = Result.Nodes.getNodeAs<clang::Stmt>("stmt");
    if (!stmt) return;

    auto loc = Result.SourceManager->getPresumedLoc(stmt->getBeginLoc());    

    const auto type_name = decl_ref->getType().getAsString();
    const auto member_name = member->getMemberNameInfo().getAsString();
    const auto file_name = std::string(loc.getFilename());
    const auto line_number = std::to_string(loc.getLine());
    const std::string url = std::string("https://elixir.bootlin.com/linux/latest/source/") + file_name + "#L" + line_number;
    
    llvm::errs() << 
    file_name << "," <<
    type_name << "," <<
    member_name << "," <<
    line_number << "," <<
    url << "," <<
    "\n";
  }
};



class PrintFunctionNamesAction : public PluginASTAction {
  std::set<std::string> ParsedTemplates;
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) override {
    Finder.addMatcher(LoopMatcher, &Printer);
    return Finder.newASTConsumer();
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    for (unsigned i = 0, e = args.size(); i != e; ++i) {
      llvm::errs() << "PrintFunctionNames arg = " << args[i] << "\n";

      // Example error handling.
      DiagnosticsEngine &D = CI.getDiagnostics();
      if (args[i] == "-an-error") {
        unsigned DiagID = D.getCustomDiagID(DiagnosticsEngine::Error,
                                            "invalid argument '%0'");
        D.Report(DiagID) << args[i];
        return false;
      } else if (args[i] == "-parse-template") {
        if (i + 1 >= e) {
          D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                     "missing -parse-template argument"));
          return false;
        }
        ++i;
        ParsedTemplates.insert(args[i]);
      }
    }
    if (!args.empty() && args[0] == "help")
      PrintHelp(llvm::errs());

    return true;
  }
  void PrintHelp(llvm::raw_ostream& ros) {
    ros << "Help for PrintFunctionNames plugin goes here\n";
  }

private:
  LoopPrinter Printer;
  MatchFinder Finder;

};

}

static FrontendPluginRegistry::Add<PrintFunctionNamesAction>
X("print-fns", "print function names");