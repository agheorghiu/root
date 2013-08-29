#ifndef ROOT_TPackage
#define ROOT_TPackage

#ifndef ROOT_TString
#include "TString.h"
#endif
#ifndef ROOT_TMacro
#include "TMacro.h"
#endif
#ifndef ROOT_MessageTypes
#include "MessageTypes.h"
#endif
#ifndef ROOT_TMD5
#include "TMD5.h"
#endif
#ifndef ROOT_TRegexp
#include "TRegexp.h"
#endif
#ifndef ROOT_TSysEvtHandler
#include "TSysEvtHandler.h"
#endif
#ifndef ROOT_TUrl
#include "TUrl.h"
#endif

#include "compiledata.h"
#include "THashTable.h"
#include "TROOT.h"
#include "TSystem.h"
#include "TError.h"
#include "TMethodCall.h"
#include "TMethodArg.h"
#include "TFunction.h"
#include "TObjString.h"
#include "TSystemDirectory.h"
#include "TSystemFile.h"
#include "TPackageUtils.h"
#include "TPackageFile.h"
#include "TPackageVersion.h"
#include "TPackageRange.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>

class TPackage : public TObject {
    private:
        TString packName; // base name of package
        TString completeName; // complete name (including version)
        TString sharedLibName; // name of shared library
        TString packLocation; // location of PAR file
        TString parFilePath; // path to PAR file
        TString contentLocation; // absolute path to package content
        TString mainDirLocation; // absolute path to main package directory
        TString allPacksLocation; // absolute path to packages repository
        TString binPath; // absolute path to binaries
        TPackageVersion *version; // package version

        TString tempDir; // temporary directory for storing package
        TString tempDirContent; // content of temporary directory (source files and META-INF)
        TString tempDirLib; // binaries in temporary directory
        TString workingDir; // current working directory

        TList *packFiles; // list of package files
        TList *deps; // list of dependencies

        Bool_t created; // package was created
        Bool_t unpacked; // package was unpacked
        Bool_t hasBuild; // package has Build script
        Bool_t hasSetup; // package has Setup macro
        Bool_t strictCheck; // strict consistency check flag

        void Init(const char *name, Int_t major, Int_t minor, const char *patch,
                        Bool_t check, const char *location, Int_t source);
        void MakeCompleteName();
        void StoreFilesInfo(Bool_t fromList = kTRUE) const;
        void StoreFilesInfo(std::ofstream &outFile) const;
        void StoreFilesInfo(const char *dir, std::ofstream &outFile) const;
        void StoreDependencies() const;
        TString *MakeBinariesDir(TString location);
        void MoveBinaryFiles(TString location, TString binLocation) const;
        Bool_t HasBuilt() const;
        Bool_t BuildFromScript(TString location) const;
        Bool_t BuildStandard(TString location) const;
        Bool_t RunSetup(TList *loadopts) const;
        Bool_t LoadUnloadLibs(Bool_t load) const;
        TString GetHeaderFiles() const;
        TString GetSourceFiles() const;
        TString GetObjectFiles() const;
        Bool_t AddFromFile(const char *fileName, const char *dest);
        Bool_t AddToINF(const char *srcFile, const char *refStr, const char *funName);
        Bool_t RemoveFromINF(const char *refStr, const char *funName);
        Bool_t CheckPackStructure() const;
        Bool_t CheckConsistency() const;
        Bool_t GenerateMakefile() const;
        Bool_t GenerateBuild() const;
        Bool_t GenerateLinkDef() const;
        Bool_t GenerateDict() const;
        void ClearDictFiles() const;
        void RemoveFromList(const char *filePath);
        void TrimPaths(const char *baseDir) const;
        void ShowContents() const;
        void ShowInfo() const;
        void ShowChecksums() const;
        void ShowDependencies() const;
        void ShowOptions() const;
        void RestorePackFiles();
        void RestoreDeps();
        static TPackage *OpenPAR(const char *parFile, Bool_t check);
        static TPackage *OpenDir(const char *dir, Bool_t check);
    public:
        TPackage(const char *name, Int_t major, Int_t minor, const char *patch = NULL,
                    Bool_t check = kFALSE, const char *location = NULL, Int_t source = 0);
        TPackage(const char *fullName, Bool_t check = kFALSE, const char *location = NULL,
                    Int_t source = 0);
        ~TPackage();

        static TPackage *Open(const char *packPath, Bool_t check = kFALSE);

        inline const char *GetPackName() const { return packName.Data(); }
        inline const char *GetPARLocation() const { return packLocation.Data(); }
        inline const char *GetMainDirLocation() const { return mainDirLocation.Data(); }
        inline const char *GetContentLocation() const { return contentLocation.Data(); }
        inline const char *GetPacksDir() const { return allPacksLocation.Data(); }
        inline Int_t GetMajor() const { return version -> GetMajorVersion(); }
        inline Int_t GetMinor() const { return version -> GetMinorVersion(); }
        inline const char *GetPatch() const { return version -> GetPatchVersion(); }
        inline TPackageVersion *GetVersion() const { return version; }

        Bool_t Add(const char *srcFile, const char *dest = NULL);
        Bool_t AddBuild(const char *srcFile);
        Bool_t AddSetup(const char *srcFile);
        Bool_t AddDep(const char *dep);
        Bool_t RemoveDep(Int_t index);
        Bool_t Remove(const char *srcFile);
        Bool_t RemoveBuild();
        Bool_t RemoveSetup();
        Bool_t Create();
        Bool_t Unpack();
        Bool_t Build();
        Bool_t Load(TList *loadopts = NULL, Bool_t recompile = kFALSE);
        Bool_t Enable(TList *loadopts = NULL, Bool_t recompile = kFALSE);
        Bool_t Unload() const;
        Bool_t IsLoaded() const;
        void Clean() const;
        void CleanAll() const;

        ULong_t Hash() const { return TString(completeName).Hash(); }
        Bool_t IsEqual(const TObject *obj) const;
        void Print(Option_t *option = "") const;
        const char *GetName() const { return completeName.Data(); }
        const char *GetPath() const;
        TList *GetDeps() const { return deps; }

        ClassDefNV(TPackage, 1);
};

#endif
