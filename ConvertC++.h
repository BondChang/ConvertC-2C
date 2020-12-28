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

/* 获取c文件的信息 */
extern "C" __declspec(dllexport) void getCFileInfo(const char *filePath,
                                                   int argc, const char **argv);
extern "C" __declspec(dllexport) void writeCOrHInfo2Xml(const char **filePathList, int fileCount, int argc,const char **argv, const char *writePath);
#endif