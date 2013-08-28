#include "TPackageUtils.h"

// helper for splitting string by delimiter char
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

// split string by delimiter char
std::vector<std::string> split(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

// remove file extension
void RemoveExtension(TString &fileName)
{
    Int_t index = fileName.Last('.');
    fileName.Remove(index);
}

// check if file is a C++ source file
Bool_t IsSourceFile(TString fileName)
{
    return (fileName.EndsWith(".cxx") || fileName.EndsWith(".cpp") || fileName.EndsWith(".cc"));
}

// clear all elements from a TList
void ClearList(TList *list)
{
    TIter next(list);
    TObject *elem;
    while ((elem = next())) {
        if (elem)
            delete elem;
    }
}

// get list of files in a directory
TList* GetFiles(const char *dirName)
{
    TSystemDirectory dir(dirName, dirName);
    return dir.GetListOfFiles();
}

// get number of files in a directory
Int_t GetNumFiles(const char *dirName)
{
    TList *files = GetFiles(dirName);
    Int_t numFiles = 0;
    if (files) {
        TSystemFile *file;
        TString fname;
        TIter next(files);
        while ((file = (TSystemFile*)next())) {
            fname = file -> GetName();
            TString path;
            path.Form("%s/%s", dirName, fname.Data());
            path.ReplaceAll("//", "/");
            if (fname != "." && fname != "..") {
                if (file -> IsDirectory())
                    numFiles += GetNumFiles(path.Data());
                else
                    numFiles++;
            }
        }
    }
    return numFiles;
}

// remove file from list
void RemoveFromList(const char *filePath, TList *list)
{
    TObject *pFile = list -> FindObject(filePath);
    list -> Remove(pFile);
}

// Recursively remove files having a certain prefix and suffix
void RecursiveRm(const char *dir, const char *prefix, const char *suffix,
                            Bool_t hasStar, Bool_t noINF, TList *list)
{
    TList *files = GetFiles(dir);
    if (files) {
        TSystemFile *file;
        TString fname;
        TIter next(files);
        while ((file = (TSystemFile*)next())) {
            fname = file -> GetName();
            TString path;
            path.Form("%s/%s", dir, fname.Data());
            path.ReplaceAll("//", "/");
            if (fname != "." && fname != ".." && (noINF == kTRUE || fname != INF_DIR)) {
                if (hasStar) {
                    if (file -> IsDirectory() && fname.BeginsWith(prefix) && fname.EndsWith(suffix))
                        RecursiveRm(path.Data(), "", "", kTRUE, noINF, list);
                    if (fname.BeginsWith(prefix) && fname.EndsWith(suffix)) {
                        gSystem -> Unlink(path.Data());
                        if (!file -> IsDirectory() && list)
                            RemoveFromList(path.Data(), list);
                    }
                } else {
                    if (file -> IsDirectory() && fname.EqualTo(prefix))
                        RecursiveRm(path.Data(), "", "", kTRUE, noINF, list);
                    if (fname.EqualTo(prefix)) {
                        gSystem -> Unlink(path.Data());
                        if (!file -> IsDirectory() && list)
                            RemoveFromList(path.Data(), list);
                    }
                }
            }
        }
    }
}

// Recursively copy files having a certain prefix and suffix
void RecursiveCp(const char *dir, const char *dest, const char *prefix,
        const char *suffix, Bool_t hasStar, Bool_t restrictInf, TList *list)
{
    TList *files = GetFiles(dir);
    if (files) {
        TSystemFile *file;
        TString fname;
        TIter next(files);
        while ((file = (TSystemFile*)next())) {
            fname = file -> GetName();
            TString path, destPath;
            path.Form("%s/%s", dir, fname.Data());
            destPath.Form("%s/%s", dest, fname.Data());
            path.ReplaceAll("//", "/");
            destPath.ReplaceAll("//", "/");
            if (fname != "." && fname != ".." && (restrictInf == kFALSE ||
                    (restrictInf == kTRUE && fname != INF_DIR))) {
                if (hasStar) {
                    if (file -> IsDirectory() && fname.BeginsWith(prefix) && fname.EndsWith(suffix)) {
                        gSystem -> MakeDirectory(destPath.Data());
                        RecursiveCp(path.Data(), destPath.Data(), "", "", kTRUE, restrictInf, list);
                    }
                    if (fname.BeginsWith(prefix) && fname.EndsWith(suffix)) {
                        FileStat_t fileInfo;
                        gSystem -> GetPathInfo(path.Data(), fileInfo);
                        gSystem -> CopyFile(path.Data(), destPath.Data());
                        gSystem -> Chmod(destPath.Data(), (UInt_t)(fileInfo.fMode));
                        if (!file -> IsDirectory() && list) {
                            TPackageFile *pFile = new TPackageFile(destPath);
                            list -> Add(pFile);
                        }
                    }
                } else {
                    if (file -> IsDirectory() && fname.EqualTo(prefix)) {
                        gSystem -> MakeDirectory(destPath.Data());
                        RecursiveCp(path.Data(), destPath.Data(), "", "", kTRUE, restrictInf, list);
                    }
                    if (fname.EqualTo(prefix)) {
                        FileStat_t fileInfo;
                        gSystem -> GetPathInfo(path.Data(), fileInfo);
                        gSystem -> CopyFile(path.Data(), destPath.Data());
                        gSystem -> Chmod(destPath.Data(), (UInt_t)(fileInfo.fMode));
                        if (!file -> IsDirectory() && list) {
                            TPackageFile *pFile = new TPackageFile(destPath);
                            list -> Add(pFile);
                        }
                    }
                }
            }
        }
    }
}

// Recursively list files in a directory
void RecursiveLs(const char *dir, Int_t depth)
{
    TList *files = GetFiles(dir);
    if (files) {
        TSystemFile *file;
        TString fname;
        TIter next(files);
        while ((file = (TSystemFile*)next())) {
            fname = file -> GetName();
            TString path;
            path.Form("%s/%s", dir, fname.Data());
            path.ReplaceAll("//", "/");
            if (fname != "." && fname != "..") {
                for (Int_t i = 0; i < depth; i++)
                    std::cout << "\t";
                std::cout << fname.Data() << std::endl;
                if (file -> IsDirectory())
                    RecursiveLs(path.Data(), depth + 1);
            }
        }
    }
}

// remove file or directory
void RmWithList(const char *file, Bool_t noINF, TList *list)
{
    const char *fileName = gSystem -> BaseName(file);
    TString dir = file;
    dir.Remove(strlen(file) - strlen(fileName));
    const char *fileDir = dir.Data();
    char *prefix, *suffix;
    Bool_t hasStar = kFALSE;
    if (TString(fileName).Contains('*')) {
        hasStar = kTRUE;
        std::vector<std::string> tokens = split(fileName, '*');
        if (tokens.size() == 1) {
            prefix = strdup(tokens[0].c_str());
            suffix = NULL;
        } else {
            prefix = strdup(tokens[0].c_str());
            suffix = strdup(tokens[1].c_str());
        }
    } else {
        prefix = strdup(fileName);
        suffix = NULL;
    }
    RecursiveRm(fileDir, prefix, suffix, hasStar, noINF, list);
    free(prefix);
    free(suffix);
}

// copy file or directory, except META-INF if specified, and add contents to List
void RCpWithList(const char *src, const char *dest, Bool_t restrictInf, TList *list)
{
    const char *fileName = gSystem -> BaseName(src);
    TString dir = src;
    dir.Remove(strlen(src) - strlen(fileName));
    const char *fileDir = dir.Data();
    char *prefix, *suffix;
    Bool_t hasStar = kFALSE;
    if (TString(fileName).Contains('*')) {
        hasStar = kTRUE;
        std::vector<std::string> tokens = split(fileName, '*');
        if (tokens.size() == 1) {
            prefix = strdup(tokens[0].c_str());
            suffix = NULL;
        } else {
            prefix = strdup(tokens[0].c_str());
            suffix = strdup(tokens[1].c_str());
        }
    } else {
        prefix = strdup(fileName);
        suffix = NULL;
    }
    RecursiveCp(fileDir, dest, prefix, suffix, hasStar, restrictInf, list);
    free(prefix);
    free(suffix);
}

// copy file or directory and add contents to List
void CpWithList(const char *src, const char *dest, TList *list)
{
    RCpWithList(src, dest, kFALSE, list);
}

// copy file or directory without adding to list
void Cp(const char *src, const char *dest)
{
    CpWithList(src, dest, NULL);
}

// remove file or directory without removing from list
void Rm(const char *file)
{
    RmWithList(file, kTRUE, NULL);
}

// list files in a directory
void Ls(const char *dir)
{
    std::cout << gSystem -> BaseName(dir) << std::endl;
    RecursiveLs(dir, 1);
}

// normalize a given path
TString NormalizePath(const char *path)
{
    TString currentDir = gSystem -> WorkingDirectory();
    if (gSystem -> ChangeDirectory(path) == kFALSE)
        return TString("");
    TString normalizedPath = gSystem -> WorkingDirectory();
    gSystem -> ChangeDirectory(currentDir.Data());
    return normalizedPath;
}
