set(LLVM_LINK_COMPONENTS support)

add_clang_executable(c++2c
  ConvertC++.cpp
  )
target_link_libraries(c++2c
  PRIVATE
  clangTooling
  clangBasic
  clangASTMatchers
  clangAST
  LLVMSupport
  clangFrontend
  clangDriver
  clangIndex
  clangLex
  clangSema
  clangSerialization
  )
