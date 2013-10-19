#ifndef ROOT_TPackageUtils
#define ROOT_TPackageUtils

#include "TSystem.h"
#include "TSystemDirectory.h"
#include "TSystemFile.h"
#include "TList.h"
#include "TPackageFile.h"
#include <stdlib.h>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

#define PACK_DIR "rootpacks"
#define ZIP_CMD "zip -r %s %s"
#define UNZIP_CMD "unzip %s -d %s"
#define LS_CMD "ls -R %s"
#define LS_CMD2 "unzip -l %s"
#define INF_DIR "META-INF"
#define LIB_DIR "lib"
#define CHECKSUMS_FILE "checksums"
#define DEPS_FILE "dependencies"
#define BUILD_SCRIPT "BUILD.SH"
#define SETUP_MACRO "SETUP.C"
#define OBJ_EXTENSION "*.o"
#define PAR_EXTENSION ".par"

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);
void RemoveExtension(TString &fileName);
Bool_t IsSourceFile(TString fileName);
void ClearList(TList *list);
TList* GetFiles(const char *dirName);
Int_t GetNumFiles(const char *dirName);
void RemoveFromList(const char *filePath, TList *list);
void RecursiveRm(const char *dir, const char *prefix, const char *suffix,
      Bool_t hasStar, Bool_t noINF, TList *list);
void RecursiveCp(const char *dir, const char *dest, const char *prefix,
      const char *suffix, Bool_t hasStar, Bool_t restrictInf, TList *list);
void RecursiveLs(const char *dir, Int_t depth);
void RmWithList(const char *file, Bool_t noINF, TList *list);
void RCpWithList(const char *src, const char *dest, Bool_t restrictInf, TList *list);
void CpWithList(const char *src, const char *dest, TList *list);
void Cp(const char *src, const char *dest);
void Rm(const char *file);
void Ls(const char *path);
TString NormalizePath(const char *path);

#endif
