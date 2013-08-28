#include "TPackageManagerDir.h"
#include "TPackageManagerMeta.h"

// initialization function used by constructor
Bool_t TPackageManagerDir::Init(const char *dirPath)
{
    TString normalizedPath = NormalizePath(dirPath);
    if (!normalizedPath.EndsWith(PACK_DIR)) {
        Error("Init", "Invalid path");
        return kFALSE;
    }
    repoPath = normalizedPath;
    availablePacks = NULL;
    return kTRUE;
}

// constructor
TPackageManagerDir::TPackageManagerDir(const char *dirPath)
{
    if (Init(dirPath) == kFALSE)
        SetBit(kInvalidObject);
//    else
//        gPackMan -> Add(this);
}

// construct list of available packages
void TPackageManagerDir::BuildList() const
{
    TList *files = GetFiles(repoPath.Data());
    if (files) {
        TSystemFile *file;
        TString fname;
        TIter next(files);
        while ((file = (TSystemFile*)next())) {
            fname = file -> GetName();
            TString path;
            path.Form("%s/%s", repoPath.Data(), fname.Data());
            if (fname != "." && fname != "..") {
                TPackage *pack = TPackage::Open(path.Data());
                if (pack != NULL && pack -> TestBit(kInvalidObject) == kFALSE)
                    availablePacks -> Add(pack);
                else
                    delete pack;
           }
        }
    }
}

// enable a specific package
Int_t TPackageManagerDir::Enable(const char *packName)
{
    if (TestBit(kInvalidObject) == kTRUE) {
        Error("Enable", "This object is invalid");
        return -1;
    }
    if (!availablePacks) {
        availablePacks = new TList();
        BuildList();
    }
    TPackage *package = (TPackage *)availablePacks -> FindObject(packName);
    if (package == NULL) {
        Error("Enable", "Package does not exist");
        return -2;
    }
    if (package -> IsLoaded() == kTRUE) {
        Info("Enable", "Package already enabled");
        return 0;
    }
    if (package -> Enable() == kFALSE) {
        Error("Enable", "Package could not be enabled");
        return -3;
    }
    Info("Enable", "Package successfully enabled");
    return 0;
}

// disable a specific package
Int_t TPackageManagerDir::Disable(const char *packName)
{
    if (TestBit(kInvalidObject) == kTRUE) {
        Error("Disable", "This object is invalid");
        return -1;
    }
    if (!availablePacks) {
        availablePacks = new TList();
        BuildList();
    }
    TPackage *package = (TPackage *)availablePacks -> FindObject(packName);
    if (package == NULL) {
        Error("Disable", "Package does not exist");
        return -2;
    }
    if (package -> IsLoaded() == kFALSE) {
        Info("Disable", "Package already disabled");
        return 0;
    }
    if (package -> Unload() == kFALSE) {
        Error("Disable", "Package could not be disabled");
        return -3;
    }
    Info("Disable", "Package successfully disabled");
    return 0;
}

// remove a specific package
Int_t TPackageManagerDir::Remove(const char *packName)
{
    if (TestBit(kInvalidObject) == kTRUE) {
        Error("Remove", "This object is invalid");
        return -1;
    }
    if (!availablePacks) {
        availablePacks = new TList();
        BuildList();
    }
    TPackage *package = (TPackage *)availablePacks -> FindObject(packName);
    if (package == NULL) {
        Error("Remove", "Package does not exist");
        return -2;
    }
    if (Disable(packName) != 0) {
        Error("Remove", "Package could not be disabled. Can't remove");
        return -3;
    }
    Rm(package -> GetPath());
    Info("Remove", "Package successfully removed");
    return 0;
}

// returns list of available packages
TList *TPackageManagerDir::List()
{
    if (TestBit(kInvalidObject) == kTRUE)
        return NULL;
    if (!availablePacks) {
        availablePacks = new TList();
        BuildList();
    }
    return availablePacks;
}

// show contents of repository
void TPackageManagerDir::ShowContents() const
{
    TList *files = GetFiles(repoPath.Data());
    if (files) {
        TSystemFile *file;
        TString fname;
        TIter next(files);
        while ((file = (TSystemFile*)next())) {
            fname = file -> GetName();
            if (fname != "." && fname != "..")
                std::cout << fname << std::endl;
        }
    }
}

// show packages in repository
void TPackageManagerDir::ShowPackages() const
{
    if (!availablePacks) {
        availablePacks = new TList();
        BuildList();
    }
    TIter next(availablePacks);
    TObject *package;
    while ((package = next())) {
        std::cout << package -> GetName() << std::endl;
    }
}


// print method inherited from TObject
void TPackageManagerDir::Print(Option_t *option) const
{
    if (TestBit(kInvalidObject) == kTRUE)
        return;
    TString opt = (const char *)option;
    if (opt == "contents") {
        ShowContents();
        return;
    }
    if (opt == "packages") {
        ShowPackages();
        return;
    }
}

Bool_t TPackageManagerDir::ResolveDeps(TPackage *package)
{
    return gPackMan -> ResolveDeps(package);
}
