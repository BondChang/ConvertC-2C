#ifndef DLLPARSER_H
#define DLLPARSER_H 
#include "clang-c\Index.h"
#include "clang/AST/APValue.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTDiagnostic.h"
#include "clang/AST/CharUnits.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/AST/TypeLoc.h"
#include "clang/Basic/Builtins.h"
#include "clang/Basic/Module.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Driver/Options.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/Utils.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Host.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"

#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Frontend/FrontendOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Parse/Parser.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Frontend/Utils.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Comment.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <fstream>
#include <cstring>
#include <iostream>
#include <list>
#include <numeric>
#include <string>
#include <stdbool.h>
/* 初始化类中的函数 */
struct InitClassFun {
	std::string name;
	std::string initCompound;
};
/* 头文件中带有函数体的函数 */
struct HaveFunBodyHpp {
	std::string funDeclare;
	std::string funBody;
};

/* 同一文件中所有带函数体的函数 */
struct AllHaveFunBodyHpp {
	std::string funPath;
	std::vector<HaveFunBodyHpp> funs;
};

/* 同一文件中所有带函数体的函数 */
struct AllFileContent {
	std::string writePath;
	std::vector<std::string> fileContent;
};


struct Comment {
	Comment(const std::string& Message, unsigned Line, unsigned Col)
		: Message(Message), Line(Line), Col(Col) { }
	std::string Message;
	unsigned Line, Col;
};
/* 获取c文件的信息 */
extern "C" __declspec(dllexport) void convertFiles(const char** sourcePath, const char** targetPath, int fileCount);

#endif