#include "ConvertC++.h"

using namespace std;
using namespace clang;
using namespace clang::tooling;

typedef list<char*> LISTVARS;
LISTVARS listVars;
extern "C" __declspec(dllexport) std::string parserExpr(Stmt * stmt, ASTContext * Context);
extern "C" __declspec(dllexport) void dealInitValue(const Expr * initExpr, std::string * initValueStr);
extern "C" __declspec(dllexport) std::string dealType(const Type * type, std::string typeStr, QualType nodeType);
int funArrayLen = 0;
bool isParserIfStmt = false;
std::vector<int> fileLocVector;
std::vector<InitClassFun> initClassFunVector;
std::vector<std::string> allConstructionMethodVector;
/* 存储所有的文件信息 */
std::vector<std::string> fileContentVector;
static llvm::cl::OptionCategory MyToolCategory("global-detect options");
class Find_Includes : public PPCallbacks {
public:
	bool has_include;

	void InclusionDirective(SourceLocation hash_loc, const Token& include_token,
		StringRef file_name, bool is_angled,
		CharSourceRange filename_range, const FileEntry* file,
		StringRef search_path, StringRef relative_path,
		const clang::Module* imported,
		SrcMgr::CharacteristicKind FileType) {
		// do something with the include
		std::cout << file_name.str();
		std::cout << "Found at least one include" << std::endl;
		has_include = true;
	}
};
bool isParserd(SourceLocation sourceLoc,
	ASTContext* Context) {
	SourceManager& sourceManager = Context->getSourceManager();
	int lineNum = sourceManager.getPresumedLoc(sourceLoc).getLine();
	std::vector<int>::iterator ret;
	ret = std::find(fileLocVector.begin(), fileLocVector.end(), lineNum);
	if (ret == fileLocVector.end()) {
		return false;
	}
	else {
		return true;
	}
}
bool judgeHasConstructionMethod(std::string className) {
	if (allConstructionMethodVector.size() > 0) {
		for (auto iter = allConstructionMethodVector.begin(); iter != allConstructionMethodVector.end(); iter++)
		{
			if (className+"_"+className == (*iter)) {
				return true;
			}
		}
	}
	return false;
}
std::string dealRecordStmt(std::string typeStr,bool isPointerType,std::string nameStr,std::string initValueStr) {
	bool isHasConstructionMethod = judgeHasConstructionMethod(typeStr);
	std::string paraName="";
	if (isPointerType) {
		paraName = nameStr;
	}
	else {
		paraName = "&"+nameStr;
	}
	std::string initStructStr = "";
	if (isHasConstructionMethod) {
		initStructStr += typeStr + "_" + typeStr + "(" + paraName + ");";
	}
	else {
		initStructStr += typeStr + "_" + "init" + "(" + paraName + ");";
	}
	if (initValueStr == "") {

		return typeStr + "* " + nameStr + ";" + "\n\t" + initStructStr ;
	}
	else {
		return typeStr + "* " + nameStr + "=" + initValueStr + ";" + "\n\t" + initStructStr ;
	}
}
std::string dealVarDeclNode(VarDecl* var) {
	std::string typeStr = var->getType().getLocalUnqualifiedType().getCanonicalType().getAsString();
	const Type* type = var->getType().getTypePtr();
	QualType nodeType = var->getType();
	if (type->getTypeClass() == 8) {
		const DecayedType* DT = type->getAs<DecayedType>();
		type = DT->getOriginalType().getTypePtr();
		nodeType = DT->getOriginalType();
	}
	typeStr = dealType(type, typeStr, nodeType);
	std::string initValueStr = "";
	const Expr* initExpr = var->getAnyInitializer();
	/* 处理全局变量的初值 */
	dealInitValue(initExpr, &initValueStr);

	std::string nameStr = var->getNameAsString();
	if (type->getTypeClass() == 36) {
		typeStr = type->getAsRecordDecl()->getNameAsString();
		return dealRecordStmt(typeStr, false,nameStr,initValueStr);
	}
	else if (type->getTypeClass() == 34) {
		QualType pointType = type->getPointeeType();
		if (pointType.getTypePtr()->isStructureOrClassType()) {
			typeStr = pointType.getTypePtr()->getAsCXXRecordDecl()->getNameAsString();
			return dealRecordStmt(typeStr, true, nameStr, initValueStr);
		}

	}

	/*if (auto *reclDecl=dyn_cast<CXXRecordDecl>(var->getType())) {
		typeStr = reclDecl->getNameAsString();
	}*/

	if (initValueStr == "") {
		return typeStr + " " + nameStr + ";";
	}
	else {
		return typeStr + " " + nameStr + "=" + initValueStr + ";";
	}

}
void addNodeInfo(SourceLocation sourceLoc,
	ASTContext* Context) {
	SourceManager& sourceManager = Context->getSourceManager();
	int lineNum = sourceManager.getPresumedLoc(sourceLoc).getLine();
	//std::string fileLoc = sourceManager.getFileLoc(sourceLoc).str();
	fileLocVector.push_back(lineNum);
}
std::string dealDim(const Type* type, std::string dimStr) {

	/* 转换成数组类型 */
	const ArrayType* arrayType = type->castAsArrayTypeUnsafe();
	/* 处理ConstantArrayType类型，获取数组的维度 */
	if (arrayType->isConstantArrayType()) {
		int dim =
			dyn_cast<ConstantArrayType>(arrayType)->getSize().getLimitedValue();
		dimStr += std::to_string(dim);
		dimStr += "]";
	}
	/* 处理IncompleteArrayType类型 */
	else if (arrayType->isIncompleteArrayType()) {
		dimStr += "]";
	}

	/* 获取数组的类型 */
	const Type* childType = arrayType->getElementType().getTypePtr();

	/* 如果是ConstantArrayType或者IncompleteArrayType类型的数组 */
	if (childType->isConstantArrayType() || childType->isIncompleteArrayType()) {
		dimStr += "[";
		return dealDim(childType, dimStr);
	}
	else {
		return dimStr;
	}
}
QualType dealArrayType(const Type* type) {

	/* 转换成数组类型 */
	const ArrayType* arrayType = type->castAsArrayTypeUnsafe();
	/* 获取数组的维度 */

	/* 获取数组的类型 */
	const Type* childType = arrayType->getElementType()
		.getLocalUnqualifiedType()
		.getCanonicalType()
		.getTypePtr();
	/* 数组类型为ConstantArrayType()，如int[3] */
	/* 数组类型为IncompleteArrayType，如int[] */
	if (childType->isConstantArrayType() || childType->isIncompleteArrayType()) {
		return dealArrayType(childType);
	}
	else {
		QualType qualType = arrayType->getElementType()
			.getLocalUnqualifiedType()
			.getCanonicalType();
		return qualType;
	}
}
QualType dealPointerType(const Type* pointerType, std::string& pointer) {

	const Type* childType = pointerType->getPointeeType()
		.getLocalUnqualifiedType()
		.getCanonicalType()
		.getTypePtr();
	/* 如果子表达式仍为指针类型 */
	if (childType->isPointerType()) {
		pointer = pointer + "*";
		return dealPointerType(childType, pointer);
	}
	/* 如果不为指针类型 */
	else {
		QualType type = pointerType->getPointeeType();
		return type;
	}
}
std::string dealQualifiers(QualType nodeType,
	std::string typeStr) {
	std::string qualifier = nodeType.getQualifiers().getAsString();
	return qualifier;
}
std::string dealType(const Type* type, std::string typeStr, QualType nodeType) {
	std::string qualifier = "";
	/* 说明有类型修饰符 */
	if (nodeType.hasQualifiers()) {
		qualifier = dealQualifiers(nodeType, typeStr);
	}
	/* 处理类型为ConstantArrayType的数组，如int[3] */
	/* 处理IncompleteArrayType类型的数组,如int[] */
	if (type->isConstantArrayType() || type->isIncompleteArrayType()) {
		/* 处理数组的类型 */
		QualType qualType = dealArrayType(type);
		typeStr = qualType.getAsString();
		/* 处理数组的维度 */
		std::string dimStr = "[";
		dimStr = dealDim(type, dimStr);
		type = qualType.getTypePtr();
		if (type->isPointerType()) {
			dealType(type, typeStr, nodeType);
		}
		typeStr = qualifier + " " + typeStr + " " + dimStr;
	}
	/* 如果是指针类型 */
	else if (type->isPointerType()) {
		/* 处理指针类型的* */
		std::string pointer = "*";
		/* 获取除*以外的类型 */
		QualType pointType = dealPointerType(type, pointer);
		typeStr = pointType.getAsString();
		/* 如果像包括const，volatile修饰符 */
		/*if (pointType.hasQualifiers()) {
			dealQualifiers(pointType, typeStr);
		}*/
		typeStr = pointType.getLocalUnqualifiedType().getCanonicalType().getAsString();
		/* 类型加指针 */
		std::string dimStr = pointer;
		typeStr = qualifier + " " + typeStr + " " + dimStr;
	}

	/* 如果不是数组 */
	else {
		std::string dimStr = "";
		typeStr = qualifier + " " + typeStr + " " + dimStr;
	}
	return typeStr;
}
bool judgeConstStIsFunStmt(const Stmt* st, ASTContext* astContext) {
	auto& parents = astContext->getParents(*st);
	if (parents.empty()) {
		return false;
	}
	/* 获取父节点 */
	const Stmt* currentSt = parents[0].get<Stmt>();
	if (!currentSt) {
		return false;
	}
	if (isa<CompoundStmt>(currentSt)) {
		return true;
	}
	else {
		return judgeConstStIsFunStmt(currentSt, astContext);
	}
	return false;
}

bool judgeConstStIsFunDecl(const Decl* decl, ASTContext* astContext) {
	auto& parents = astContext->getParents(*decl);
	if (parents.empty()) {
		return false;
	}
	/* 获取父节点 */
	const Stmt* currentSt = parents[0].get<Stmt>();
	const Decl* currentDecl = parents[0].get<Decl>();
	if (!currentSt) {
		if (!currentDecl) {
			return false;
		}
		else {
			return judgeConstStIsFunDecl(currentDecl, astContext);
		}
	}
	if (isa<CompoundStmt>(currentSt)) {
		return true;
	}
	else {
		return judgeConstStIsFunStmt(currentSt, astContext);
	}
	return false;
}

bool judgeStIsFunStmt(Stmt* st, ASTContext* astContext) {
	/* 如果直接是函数或CompoundStmt节点，不用看parent */
	if (isa<CompoundStmt>(st)) {
		return true;
	}
	auto& parents = astContext->getParents(*st);
	if (parents.empty()) {
		return false;
	}
	/* 获取父节点 */
	const Stmt* currentSt = parents[0].get<Stmt>();
	const Decl* curentDecl = parents[0].get<Decl>();
	if (!currentSt) {
		if (!curentDecl) {
			return false;
		}
		else {
			return judgeConstStIsFunDecl(curentDecl, astContext);
		}
	}

	if (isa<CompoundStmt>(currentSt)) {
		return true;
	}
	else {
		return judgeConstStIsFunStmt(currentSt, astContext);
	}
	return false;
}

bool judgeIsHaveInitFun(CXXRecordDecl* recodeDecl) {
	std::string recodeName = recodeDecl->getNameAsString();
	RecordDecl::field_iterator jt;
	for (jt = recodeDecl->field_begin();
		jt != recodeDecl->field_end(); jt++) {
		auto a = jt->getType();
		auto c = a.getTypePtr();
	}
	//auto a=recodeDecl->begin();

	return true;
}
bool judgeIsClassFun(FunctionDecl* func) {
	auto declNode = func->getParent();
	if (dyn_cast<CXXRecordDecl>(declNode)) {
		return true;
	}
	else {
		return false;
	}
}

/* 处理结构体信息 */
void dealVarRecordDeclNode(CXXRecordDecl const* varRecordDeclNode, ASTContext* Context) {
	std::string recodeStr = "";
	std::string nameStr = "";
	std::string qualifier = "";
	if (nameStr.empty() && varRecordDeclNode->getTypedefNameForAnonDecl()) {
		/* 获取结构体的typedef名称 */
		nameStr = varRecordDeclNode->getTypedefNameForAnonDecl()->getNameAsString();
	}
	if (nameStr.empty()) {
		/* 直接获取结构体的名字 */
		nameStr = varRecordDeclNode->getNameAsString();
	}
	else {
		qualifier = "typedef ";
	}
	std::string structType = "";
	if (varRecordDeclNode->isUnion()) {
		structType = "union ";
	}
	else {
		structType = "struct ";
	}
	if (varRecordDeclNode->isClass()) {
		qualifier = "typedef ";
	}
	if (varRecordDeclNode->isClass()) {
		recodeStr += qualifier + structType + "Class_" + nameStr + "\n" + "{\n\t";
	}
	else {
		recodeStr += qualifier + structType + nameStr + "\n" + "{\n\t";
	}

	/* 记录结构体的field数量 */
	RecordDecl::field_iterator jt;
	std::string initClassFunStr = "";
	for (jt = varRecordDeclNode->field_begin();
		jt != varRecordDeclNode->field_end(); jt++) {
		FieldDecl* field = *jt;
		addNodeInfo(field->getBeginLoc(), Context);
		std::string fieldNameStr = jt->getNameAsString();
		/* 获取全局变量的类型，为总类型 */
		std::string fieldTypeStr = jt->getType().getLocalUnqualifiedType().getCanonicalType().getAsString();
		/* 将全局变量的类型转成Type类型,便于后续判断是否为数组 */
		const Type* type = jt->getType().getTypePtr();
		QualType nodeType = jt->getType();
		dealType(type, fieldTypeStr, nodeType);
		recodeStr += fieldTypeStr + " " + fieldNameStr + ";\n\t";
		std::string initValueStr = "";
		const Expr* initExpr = field->getInClassInitializer();;
		/* 处理全局变量的初值 */
		dealInitValue(initExpr, &initValueStr);
		if (!(initValueStr == "")) {
			initClassFunStr += "p" + nameStr + "->" + fieldNameStr + "=" + initValueStr + ";\n\t";
		}
	}
	if (!(initClassFunStr == "")) {
		InitClassFun initClassfun;
		initClassfun.name = nameStr;
		initClassfun.initCompound = initClassFunStr;
		initClassFunVector.push_back(initClassfun);
	}
	if (varRecordDeclNode->isClass()) {
		recodeStr += "\r}" + nameStr + ";";
	}
	else {
		recodeStr += "\r};";
	}
	fileContentVector.push_back(recodeStr);

}

std::string getInitStrInVector(std::string funcName) {
	if (initClassFunVector.size() > 0) {
		for (auto iter = initClassFunVector.begin(); iter != initClassFunVector.end(); iter++)
		{
			if (funcName == (*iter).name + "_" + (*iter).name) {
				std::string initStr = (*iter).initCompound;
				initClassFunVector.erase(iter);
				return initStr;
			}
		}
	}
	return "";
}
class FindNamedClassVisitor
	: public RecursiveASTVisitor<FindNamedClassVisitor> {
public:
	explicit FindNamedClassVisitor(ASTContext* Context) : Context(Context) {}

private:
	ASTContext* astContext; // used for getting additional AST info

public:
	virtual bool VisitVarDecl(VarDecl* var) {
		auto isInFunction = var->getParentFunctionOrMethod();
		if (!isInFunction) {
			if (!isParserd(var->getBeginLoc(), Context)) {
				addNodeInfo(var->getBeginLoc(), Context);
				fileContentVector.push_back(dealVarDeclNode(var));
			}
		}
		return true;
	}
	virtual bool VisitFunctionDecl(FunctionDecl* func) {
		if (func->isCanonicalDecl()) {
			bool isClassFun = judgeIsClassFun(func);
			std::string funStr = "";
			std::string funcName = func->getNameInfo().getName().getAsString();
			std::string funcType = func->getType().getAsString();
			std::string returTypeStr = func->getReturnType().getAsString();
			int paraNum = func->getNumParams();
			if (isClassFun) {
				CXXRecordDecl* cxxRecordDecl = dyn_cast<CXXRecordDecl>(func->getParent());
				auto className = cxxRecordDecl->getNameAsString();
				funcName = className + "_" + funcName;
				funStr += returTypeStr + " " + funcName + "(";
				funStr += className + " *p" + className;
				if (paraNum != 0) {
					funStr += ",";
				}
			}
			else {
				funStr += returTypeStr + " " + funcName + "(";
			}
			if (paraNum != 0) {
				for (int i = 0; i < paraNum; i++) {
					std::string nameStr = func->getParamDecl(i)->getNameAsString();
					/* 获取函数参数的类型 */
					std::string typeStr = func->getParamDecl(i)->getType().getLocalUnqualifiedType().getCanonicalType().getAsString();
					/* 将函数参数的类型转换为Type,方便后续判断是否为数组类型 */
					const Type* type = func->getParamDecl(i)->getType().getTypePtr();
					// std::cout << type->getTypeClassName() << type->getTypeClass()<< "\n";
					/* 说明是Decayed类型,即本来是数组类型，被clang转换成指针类型 */
					QualType nodeType = func->getParamDecl(i)->getType();
					if (type->getTypeClass() == 8) {
						const DecayedType* DT = type->getAs<DecayedType>();
						type = DT->getOriginalType().getTypePtr();
						nodeType = DT->getOriginalType();
					}
					typeStr = dealType(type, typeStr, nodeType);
					if (i == paraNum - 1) {
						funStr += typeStr + " " + nameStr;
					}
					else {
						funStr += typeStr + " " + nameStr + ",";
					}

				}
			}
			funStr += ")\n{\n\t";
			/* 判断解析到的函数是否是类的构造函数且在Vector中 */
			std::string initClassFunStr = getInitStrInVector(funcName);
			if (!(initClassFunStr == "")) {
				allConstructionMethodVector.push_back(funcName);
				funStr += initClassFunStr;
			}
			/* 如果是类内的函数，则应该增加将类转换后的结构体 */
			Stmt* comStmtBody = func->getBody();
			if (comStmtBody) {
				std::string comStmtBodyStr = parserExpr(comStmtBody, Context);
				funStr += comStmtBodyStr + "\r}\n";
			}
			else {
				funStr += "\r};";
			}

			fileContentVector.push_back(funStr);
		}
		return true;
	}

	virtual bool VisitStmt(Stmt* st) {
		bool isFunctionStmt = judgeStIsFunStmt(st, Context);
		SourceManager& sourceManager = Context->getSourceManager();
		/* 如果不是函数中的stmt，在进行解析 */
		if (!isFunctionStmt) {
			if (!isParserd(st->getBeginLoc(), Context)) {
				std::string stmtStr = parserExpr(st, Context);
				fileContentVector.push_back(stmtStr);
			}
		}
		return true;
	}

	virtual bool VisitCXXRecordDecl(CXXRecordDecl* recodeDecl) {
		judgeIsHaveInitFun(recodeDecl);
		dealVarRecordDeclNode(recodeDecl, Context);
		return true;
	}

private:
	ASTContext* Context;
};
/* 设置默认值 */
void getDefaultVaule(const ImplicitValueInitExpr* iV,
	std::string* defaultValueStr) {
	QualType ivType = iV->getType();
	/* double与float */
	if (ivType->isConstantArrayType() || ivType->isIncompleteArrayType()) {
		*defaultValueStr += "[" + std::to_string(0) + "]";
	}
	/* int */
	else if (ivType->isStructureType()) {
		*defaultValueStr += "{" + std::to_string(0) + "}";
	}
	else {
		*defaultValueStr += std::to_string(0);
	}
}
/* 处理全局变量的初值 */
void dealInitValue(const Expr* initExpr, std::string* initValueStr) {
	if (!(initExpr == NULL)) {
		/* double或float类型的初值 */
		if (isa<FloatingLiteral>(initExpr)) {
			const FloatingLiteral* fL = dyn_cast<FloatingLiteral>(initExpr);
			double initValue = fL->getValueAsApproximateDouble();
			/* 将float/double强转为String */
			*initValueStr += std::to_string(initValue);
		}
		/* int类型的value */
		if (isa<IntegerLiteral>(initExpr)) {
			const IntegerLiteral* iL = dyn_cast<IntegerLiteral>(initExpr);
			/* 将int强转为String */
			*initValueStr += std::to_string(iL->getValue().getSExtValue());
		}
		if (isa<ImplicitCastExpr>(initExpr)) {
			const ImplicitCastExpr* iCE = dyn_cast<ImplicitCastExpr>(initExpr);
			auto subExpr = iCE->getSubExpr();
			dealInitValue(subExpr, initValueStr);
		}
		/* 没有值的情况 */
		if (isa<ImplicitValueInitExpr>(initExpr)) {
			std::string defaultValueStr = "";
			const ImplicitValueInitExpr* iV =
				dyn_cast<ImplicitValueInitExpr>(initExpr);
			/* 将int强转为String */
			getDefaultVaule(iV, &defaultValueStr);
			*initValueStr += defaultValueStr;
		}
		/* 数组 */
		if (isa<InitListExpr>(initExpr)) {
			const InitListExpr* iLE = dyn_cast<InitListExpr>(initExpr);
			SourceLocation Loc = iLE->getSourceRange().getBegin();
			signed NumInits = iLE->getNumInits();
			// const Expr *temp = iLE->getInit(0);
			QualType initExprType = initExpr->getType();
			if (initExprType->isStructureType()) {
				*initValueStr += "{";
			}
			else {
				*initValueStr += "[";
			}

			for (int i = 0; i < NumInits; i++) {
				dealInitValue(iLE->getInit(i), initValueStr);
				*initValueStr += ",";
			}
			int num = initValueStr->length();
			if (num > 0) {
				*initValueStr = initValueStr->substr(0, num - 1);
			}
			if (initExprType->isStructureType()) {
				*initValueStr += "}";
			}
			else {
				*initValueStr += "]";
			}
		}
	}
}
void dealSubIfStmt(Stmt* subOtherStmt, std::string& otherStmt, ASTContext* Context) {
	IfStmt* elseIfStmt = dyn_cast<IfStmt>(subOtherStmt);
	/* if语句的条件 */
	auto* elseIfCondExpr = elseIfStmt->getCond();
	std::string elseIfCondStr = parserExpr(elseIfCondExpr, Context);
	/* if语句体 */
	Stmt* elseIfComp = elseIfStmt->getThen();
	std::string rootIfCompStr = parserExpr(elseIfComp, Context);
	otherStmt += "else if(" + elseIfCondStr + "){" + "\n" + rootIfCompStr + "}" + "\n";
	/* 获取其他的else-if/else语句 */
	Stmt* subElseIfStmt = elseIfStmt->getElse();
	if (subElseIfStmt) {
		if (IfStmt* subIfStmt = dyn_cast<IfStmt>(subElseIfStmt)) {
			dealSubIfStmt(subIfStmt, otherStmt, Context);
		}
		else if (CompoundStmt* elseStmt = dyn_cast<CompoundStmt>(subElseIfStmt)) {
			std::string elseCondStr = parserExpr(elseStmt, Context);
			otherStmt += "else\n{" + elseCondStr + "}" + "\n";
		}
	}
}
std::string parserExpr(Stmt* stmt, ASTContext* Context) {

	addNodeInfo(stmt->getBeginLoc(), Context);
	switch (stmt->getStmtClass()) {
	case Expr::ImplicitCastExprClass:
	case Expr::CStyleCastExprClass:
	case Expr::CXXFunctionalCastExprClass:
	case Expr::CXXStaticCastExprClass:
	case Expr::CXXReinterpretCastExprClass:
	case Expr::CXXConstCastExprClass:
	{
		return parserExpr(cast<CastExpr>(stmt)->getSubExpr(), Context);
	}
	case Expr::SwitchStmtClass: {
		SwitchStmt* switchStmt = dyn_cast<SwitchStmt>(stmt);
		auto switchStmtCond = switchStmt->getCond();
		auto switchStmtBody = switchStmt->getBody();
		return "switch(" + parserExpr(switchStmtCond, Context) + ")\n\t{\n\t" + parserExpr(switchStmtBody, Context) + "\r}";
	}
	case Expr::CaseStmtClass: {
		CaseStmt* caseStmt = dyn_cast<CaseStmt>(stmt);
		return "case " + parserExpr(caseStmt->getLHS(), Context) + " :\n\t\t" + parserExpr(caseStmt->getSubStmt(), Context);
	}
	case Expr::BreakStmtClass: {
		return "\tbreak;\n";
	}
	case Expr::DefaultStmtClass: {
		DefaultStmt* defaultStmt = dyn_cast<DefaultStmt>(stmt);
		defaultStmt->getSubStmt();
		return "default :\n\t\t" + parserExpr(defaultStmt->getSubStmt(), Context) + ";\n";
	}
	case Expr::ConstantExprClass: {
		ConstantExpr* constantExpr = dyn_cast<ConstantExpr>(stmt);
		return parserExpr(constantExpr->getSubExpr(), Context);
	}
	case Expr::BinaryOperatorClass: {
		BinaryOperator* binaryOperator = dyn_cast<BinaryOperator>(stmt);
		Expr* leftBinaryOperator = binaryOperator->getLHS();
		Expr* rightBinaryOperator = binaryOperator->getRHS();
		addNodeInfo(binaryOperator->getOperatorLoc(), Context);
		auto c = binaryOperator->getOpcode();
		switch (binaryOperator->getOpcode()) {
		case BO_Add: {
			return parserExpr(leftBinaryOperator, Context) + "+" +
				parserExpr(rightBinaryOperator, Context);
		}
		case BO_Sub: {
			return parserExpr(leftBinaryOperator, Context) + "-" +
				parserExpr(rightBinaryOperator, Context);
		}
		case BO_Mul: {
			return parserExpr(leftBinaryOperator, Context) + "*" +
				parserExpr(rightBinaryOperator, Context);
		}
		case BO_Div: {
			return parserExpr(leftBinaryOperator, Context) + "/" +
				parserExpr(rightBinaryOperator, Context);
		}
		case BO_LT: {
			return parserExpr(leftBinaryOperator, Context) + "<" +
				parserExpr(rightBinaryOperator, Context);
		}
		case BO_GT: {
			return parserExpr(leftBinaryOperator, Context) + ">" +
				parserExpr(rightBinaryOperator, Context);
		}
		case BO_LE: {
			return parserExpr(leftBinaryOperator, Context) + "<=" +
				parserExpr(rightBinaryOperator, Context);
		}
		case BO_GE: {
			return parserExpr(leftBinaryOperator, Context) + ">=" +
				parserExpr(rightBinaryOperator, Context);
		}
		case BO_Assign: {
			return parserExpr(leftBinaryOperator, Context) + "=" +
				parserExpr(rightBinaryOperator, Context);
		}
		default: {
			break;
		}

		}
	}
	case Expr::MemberExprClass: {
		MemberExpr* memberExpr = dyn_cast<MemberExpr>(stmt);
		std::string className = "";
		auto cxxThisExprNode = memberExpr->getBase();
		className = parserExpr(cxxThisExprNode, Context);
		auto fildName = memberExpr->getMemberDecl()->getDeclName().getAsString();
		auto stmtClassType = cxxThisExprNode->getStmtClass();
		if (stmtClassType == Expr::CXXThisExprClass) {
			return "p" + className + "." + fildName;
		}
		else {
			return  className + fildName;
		}
	}
	case Expr::CXXThisExprClass: {
		CXXThisExpr* cxxThisExpr = dyn_cast<CXXThisExpr>(stmt);
		auto className = cxxThisExpr->getType().getBaseTypeIdentifier()->getName();
		return className;
	}
	case Expr::IntegerLiteralClass: {
		IntegerLiteral* integerLiteral = dyn_cast<IntegerLiteral>(stmt);
		int intValue = integerLiteral->getValue().getZExtValue();
		return to_string(intValue);


	}case Expr::FloatingLiteralClass: {
		FloatingLiteral* floatingLiteral = dyn_cast<FloatingLiteral>(stmt);
		llvm::SmallVector<char, 32> floatValue;
		floatingLiteral->getValue().toString(floatValue, 32, 0);
		return floatValue.data();
	}
	case Expr::IfStmtClass: {
		IfStmt* rootIfStmt = dyn_cast<IfStmt>(stmt);
		/* if语句的条件 */
		auto* rootIfCondExpr = rootIfStmt->getCond();
		std::string rootIfCondStr = parserExpr(rootIfCondExpr, Context);
		/* if语句体 */
		Stmt* rootIfComp = rootIfStmt->getThen();
		std::string rootIfCompStr = parserExpr(rootIfComp, Context);
		/* 获取其他的else-if/else语句 */
		Stmt* subOtherStmt = rootIfStmt->getElse();
		/* 说明至少含有一个else语句 */
		if (subOtherStmt) {
			std::string otherStmt;
			/* 说明是if-elseif-else语句*/
			if (IfStmt* subIfStmt = dyn_cast<IfStmt>(subOtherStmt)) {
				dealSubIfStmt(subOtherStmt, otherStmt, Context);
				auto a = "if(" + rootIfCondStr + "){" + "\n" + rootIfCompStr + "}" + "\n" + otherStmt;
				return a;
			}
			/* 说明是else语句 */
			else if (CompoundStmt* elseStmt = dyn_cast<CompoundStmt>(subOtherStmt)) {
				std::string elseCondStr = parserExpr(elseStmt, Context);
				return "if(" + rootIfCondStr + "){" + "\n" + rootIfCompStr + "}" + "\n" + "else\n{" + elseCondStr + "}" + "\n";
			}
		}
	}
	case Expr::CompoundStmtClass: {
		std::string compStmtBodyStr;
		CompoundStmt* compStmt = dyn_cast<CompoundStmt>(stmt);
		for (auto compStmtChild = compStmt->child_begin();
			compStmtChild != compStmt->child_end(); compStmtChild++) {
			Stmt* exprStmt = *compStmtChild;
			std::string singleCompStmtBodyStr = parserExpr(exprStmt, Context);
			if (singleCompStmtBodyStr.find(";") != std::string::npos) {
				compStmtBodyStr += singleCompStmtBodyStr + "\n\t";
			}
			else {
				compStmtBodyStr += singleCompStmtBodyStr + ";\n\t";
			}

		}
		return compStmtBodyStr;

	}
	case Expr::ForStmtClass: {
		ForStmt* forStmt = dyn_cast<ForStmt>(stmt);
		Stmt* forInit = forStmt->getInit();
		Expr* forCond = forStmt->getCond();
		Expr* forInc = forStmt->getInc();
		std::string forInitStr = parserExpr(forInit, Context);
		std::string forCondStr = parserExpr(forCond, Context);
		std::string forIncStr = parserExpr(forInc, Context);
		Stmt* forBody = forStmt->getBody();
		std::string forBodyStr = parserExpr(forBody, Context);
		return "for(" + forInitStr + ";" + forCondStr + ";" + forIncStr + "){\n" + forBodyStr + "}";
	}
	case Expr::UnaryOperatorClass: {
		UnaryOperator* unaryOperatorExpr = dyn_cast<UnaryOperator>(stmt);
		addNodeInfo(unaryOperatorExpr->getBeginLoc(), Context);
		switch (unaryOperatorExpr->getOpcode()) {
		case UO_PostInc: {
			return parserExpr(unaryOperatorExpr->getSubExpr(), Context) + "++";
		}
		case UO_PostDec: {
			return parserExpr(unaryOperatorExpr->getSubExpr(), Context) + "--";
		}
		}
		break;
	}
	case Expr::DeclRefExprClass: {

		DeclRefExpr* declRefExpr = dyn_cast<DeclRefExpr>(stmt);
		ValueDecl* valueDecl = declRefExpr->getDecl();
		addNodeInfo(valueDecl->getBeginLoc(), Context);
		auto type = valueDecl->getType();
		if (type->getTypeClass() == 36) {
			return type->getAsRecordDecl()->getNameAsString() + ".";
		}
		else if (type->getTypeClass() == 34) {
			QualType pointType = type->getPointeeType();
			if (pointType.getTypePtr()->isStructureOrClassType()) {
				return pointType.getTypePtr()->getAsCXXRecordDecl()->getNameAsString() + "->";
			}

		}

		return valueDecl->getNameAsString();
	}
	case Expr::WhileStmtClass: {
		WhileStmt* whileStmt = dyn_cast<WhileStmt>(stmt);
		Expr* whileCond = whileStmt->getCond();
		std::string whileCondStr = parserExpr(whileCond, Context);
		Stmt* whileBody = whileStmt->getBody();
		std::string whileBodyStr = parserExpr(whileBody, Context);
		return "while(" + whileCondStr + "){\n" + whileBodyStr + "}";
	}
	case Expr::ReturnStmtClass: {
		ReturnStmt* returnStmt = dyn_cast<ReturnStmt>(stmt);
		Expr* expr = returnStmt->getRetValue();
		return "return " + parserExpr(expr, Context) + ";";
	}
							  /*case Expr::CXXMemberCallExprClass: {
								  CXXMemberCallExpr* cXXMemberCallExpr = dyn_cast<CXXMemberCallExpr>(stmt);
								  FunctionDecl* cXX_func_decl =cXXMemberCallExpr->getDirectCallee();
							  }*/
	case Expr::CallExprClass:
	case Expr::CXXMemberCallExprClass:
	{
		CallExpr* call = dyn_cast<CallExpr>(stmt);
		auto func_decl = call->getCallee();
		auto funcCall = parserExpr(func_decl, Context);
		std::string callArgsStr = "";
		for (int i = 0, j = call->getNumArgs(); i < j; i++) {
			Expr* callArg = call->getArg(i);
			if (i == call->getNumArgs() - 1) {
				callArgsStr += parserExpr(callArg, Context);
			}
			else {
				callArgsStr += parserExpr(callArg, Context) + ",";
			}

		}
		return funcCall + "(" + callArgsStr + ")";
	}
	case Expr::DeclStmtClass: {
		DeclStmt* declStmt = dyn_cast<DeclStmt>(stmt);
		auto declExprNode = declStmt->getSingleDecl();
		addNodeInfo(declExprNode->getBeginLoc(), Context);
		if (VarDecl* varDeclNode = dyn_cast<VarDecl>(declExprNode)) {
			return dealVarDeclNode(varDeclNode);
		}

		//for (auto declChild = declStmt->child_begin(); declChild != declStmt->child_end(); declChild++) {
		//	Stmt* singleDeclStmt = *declChild;
		//	std::cout << "\n";
		//	/*if (VarDecl* vd = dyn_cast<VarDecl>(singleDeclStmt)) {
		//		std::cout << vd->getName().str() << std::endl;
		//	}*/
		//	return "123";
		//}
		break;
	}

	}

}
class FindNamedClassConsumer : public clang::ASTConsumer {
public:
	explicit FindNamedClassConsumer(ASTContext* Context) : Visitor(Context) {}

	virtual void HandleTranslationUnit(clang::ASTContext& Context) {
		Visitor.TraverseDecl(Context.getTranslationUnitDecl());
	}

private:
	FindNamedClassVisitor Visitor;
};

class FindNamedClassAction : public clang::ASTFrontendAction {
public:
	virtual std::unique_ptr<clang::ASTConsumer>
		CreateASTConsumer(clang::CompilerInstance& Compiler, llvm::StringRef InFile) {
		Preprocessor& pp = Compiler.getPreprocessor();
		Find_Includes* find_includes_callback =
			static_cast<Find_Includes*>(pp.getPPCallbacks());

		// do whatever you want with the callback now
		if (find_includes_callback->has_include) {
			std::cout << "Found at least one include" << std::endl;
		}
		return std::unique_ptr<clang::ASTConsumer>(
			new FindNamedClassConsumer(&Compiler.getASTContext()));
	}
	bool BeginSourceFileAction(CompilerInstance& ci) {
		std::unique_ptr<Find_Includes> find_includes_callback(new Find_Includes());

		Preprocessor& pp = ci.getPreprocessor();
		pp.addPPCallbacks(std::move(find_includes_callback));

		return true;
	}
	void EndSourceFileAction() {
		CompilerInstance& ci = getCompilerInstance();
		Preprocessor& pp = ci.getPreprocessor();
		Find_Includes* find_includes_callback =
			static_cast<Find_Includes*>(pp.getPPCallbacks());

		// do whatever you want with the callback now
		if (find_includes_callback->has_include) {
			std::cout << "Found at least one include" << std::endl;
		}
	}
};
int main(int argc, const char** argv) {
	int a = 3;
	const char* argvs1212[3] = { "", "", "" };
	CommonOptionsParser OptionsParser(a, argvs1212, MyToolCategory);
	// run the Clang Tool, creating a new FrontendAction (explained below)
	std::vector<std::string> fileList;
	fileList.push_back("C:\\Users\\bondc\\Desktop\\m.cpp");
	ClangTool Tool(OptionsParser.getCompilations(), fileList);
	int result = Tool.run(newFrontendActionFactory<FindNamedClassAction>().get());
	if (initClassFunVector.size() > 0) {
		for (auto iter = initClassFunVector.begin(); iter != initClassFunVector.end(); iter++)
		{
			std::string initFunStr = "";
			initFunStr += "void " + (*iter).name + "_init(" + (*iter).name + "* " + "p" + (*iter).name + ")\n{\t" + (*iter).initCompound + "\r}";
			fileContentVector.push_back(initFunStr);
		}
	}

	std::ofstream fout;
	fout.open("C:\\Users\\bondc\\Desktop\\a.c");
	if (fileContentVector.size() > 0) {
		for (auto iter = fileContentVector.begin(); iter != fileContentVector.end(); iter++)
		{
			fout << *iter << endl;
		}
	}
	fout.close();
	return result;
}