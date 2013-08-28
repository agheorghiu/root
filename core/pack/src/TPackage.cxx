#include "TPackage.h"
#include "TPackageManagerMeta.h"

void TPackage::MakeCompleteName()
{
    if (TString(GetPatch()).Length() > 0)
        completeName.Form("%s-%d.%d.%s", packName.Data(), GetMajor(),
                GetMinor(), GetPatch());
    else
        completeName.Form("%s-%d.%d", packName.Data(), GetMajor(), GetMinor());
}

// initializer function which is used by the constructor to create objects
void TPackage::Init(const char *name, Int_t major, Int_t minor, const char *patch,
                Bool_t check, const char *location, Int_t source)
{
    packName = name;
    packLocation = location;
    version = new TPackageVersion(packName, major, minor, TString(patch));
    packFiles = new TList();
    deps = new TList();
    created = kFALSE;
    unpacked = kFALSE;
    hasBuild = kFALSE;
    hasSetup = kFALSE;
    strictCheck = check;
    workingDir = gSystem -> WorkingDirectory();
    MakeCompleteName();
    sharedLibName.Form("lib%s", completeName.Data());
    allPacksLocation.Form("%s/%s", workingDir.Data(), PACK_DIR);
    mainDirLocation.Form("%s/%s", allPacksLocation.Data(), completeName.Data());
    contentLocation.Form("%s/%s", mainDirLocation.Data(), packName.Data());
    TString odbg;
    if (TString(ROOTBUILD) == TString("debug"))
        odbg.Form("-dbg");
    binPath.Form("%s/%s/ROOT-%d/%s-%s%s", mainDirLocation.Data(), LIB_DIR, gROOT -> GetVersionInt(),
            gSystem -> GetBuildArch(), gSystem -> GetBuildCompilerVersion(), odbg.Data());
    if (packLocation.Length() == 0)
        packLocation = workingDir;
    parFilePath.Form("%s/%s%s", packLocation.Data(), completeName.Data(), PAR_EXTENSION);
    tempDir.Form("%s/%s", gSystem -> TempDirectory(), completeName.Data());
    tempDirContent.Form("%s/%s", tempDir.Data(), packName.Data());
    tempDirLib.Form("%s/%s", tempDir.Data(), LIB_DIR);
    Rm(tempDir.Data());

    switch (source) {
        case 0: { // must be created
            gSystem -> MakeDirectory(tempDir.Data());
            gSystem -> MakeDirectory(tempDirContent.Data());
            gSystem -> MakeDirectory(tempDirLib.Data());
            TString strTempDir;
            strTempDir.Form("%s/%s", tempDirContent.Data(), INF_DIR);
            gSystem -> MakeDirectory(strTempDir.Data());
            break;
        }
        case 1: { // from archive
            created = kTRUE;
            break;
        }
        case 2: { // from directory
            created = kTRUE;
            unpacked = kTRUE;
            if (CheckConsistency() == kFALSE) {
                if (strictCheck == kTRUE) {
                    SetBit(kInvalidObject);
                    return;
                } else {
                    std::cout << "Warning: This package " << completeName.Data() <<
                                                        " is not consistent" << std::endl;
                    TString checksumsFile;
                    checksumsFile.Form("%s/%s/%s", contentLocation.Data(), INF_DIR, CHECKSUMS_FILE);
                    Rm(checksumsFile.Data());
                    StoreFilesInfo(kFALSE);
                }
            }
            RestorePackFiles();
            RestoreDeps();
            TString buildLocation, setupLocation;
            buildLocation.Form("%s/%s/%s", contentLocation.Data(), INF_DIR, BUILD_SCRIPT);
            setupLocation.Form("%s/%s/%s", contentLocation.Data(), INF_DIR, SETUP_MACRO);
            if (gSystem -> AccessPathName(buildLocation.Data()) == kFALSE)
                hasBuild = kTRUE;
            if (gSystem -> AccessPathName(setupLocation.Data()) == kFALSE)
                hasSetup = kTRUE;
            break;
        }
    }
}

// first constructor for creating the package
TPackage::TPackage(const char *name, Int_t major, Int_t minor, const char *patch,
        Bool_t check, const char *location, Int_t source)
{
    Init(name, major, minor, patch, check, location, source);
}

// second constructor for creating the package
TPackage::TPackage(const char *fullName, Bool_t check, const char *location, Int_t source)
{
    TPackageVersion *ver = new TPackageVersion(fullName);
    packFiles = NULL;
    deps = NULL;
    version = NULL;
    if (ver -> TestBit(kInvalidObject)) {
        SetBit(kInvalidObject);
        delete ver;
        return;
    }
    Init(ver -> GetPackName(), ver -> GetMajorVersion(), ver -> GetMinorVersion(),
                ver -> GetPatchVersion(), check, location, source);
    delete ver;
}

// destructor
TPackage::~TPackage()
{
    if (packFiles) {
        ClearList(packFiles);
        delete packFiles;
    }
    if (deps) {
        ClearList(deps);
        delete deps;
    }
    if (version)
        delete version;
}

// returns actual path to package (PAR location if unpacked or main directory location of package)
const char *TPackage::GetPath() const
{
    if (created == kFALSE)
        return tempDir.Data();
    if (unpacked == kFALSE)
        return parFilePath.Data();
    return mainDirLocation.Data();
}

// two TPackages are equal if they have the same name and have the same version
Bool_t TPackage::IsEqual(const TObject *obj) const
{
    TString str;
    str.Form("%s%d%d", ((TPackage*)obj) -> GetName(),
            ((TPackage*)obj) -> GetMajor(),
            ((TPackage*)obj) -> GetMinor());
    return completeName == str;
}

// add files to the TPackage archive
Bool_t TPackage::Add(const char *srcFile, const char *dest)
{
    TString destStr = dest;
    TString path = srcFile;
    if (created == kTRUE) {
        Error("Add", "Package already created. Cannot add!");
        return kFALSE;
    }
    if (destStr.Contains(INF_DIR)) {
        Error("Add", "%s is a reserved directory", INF_DIR);
        return kFALSE;
    }
    if (path.BeginsWith("@"))
        return AddFromFile(path(1, path.Length() - 1).Data(), dest);
    if (path.Contains("*") && path.First('*') > path.Last('/')) {
        Int_t index = path.Last('/');
        path.Remove(index);
    }
    if (gSystem -> AccessPathName(path.Data(), kReadPermission) == kTRUE) {
        Error("Add", "%s does not exist or cannot be accessed", srcFile);
        return kFALSE;
    }
    char *destination;
    if (dest == NULL)
        destination = strdup(tempDirContent.Data()); // base dir
    else {
        if (gSystem -> IsAbsoluteFileName(dest)) {
            Error("Add", "Cannot have absolute destination");
            return kFALSE;
        } else { // local dir
            TString strDest;
            strDest = Form("%s/%s", tempDirContent.Data(), dest);
            destination = strdup(strDest.Data());
        }
    }
    if (gSystem -> AccessPathName(destination) == kTRUE) // cannot access destination
        if (gSystem -> MakeDirectory(destination) != 0) {
            Error("Add", "Could not make directory %s", destination);
            free(destination);
            return kFALSE;
        }
    TString targetFile;
    if (gSystem -> IsAbsoluteFileName(srcFile))
        targetFile.Form("%s/%s", gSystem -> WorkingDirectory(), srcFile);
    else
        targetFile = srcFile;
    RCpWithList(targetFile.Data(), destination, kTRUE, packFiles);
    Info("Add", "Added %s to package", srcFile);
    free(destination);
    return kTRUE;
}

// add files to package from a list specified in a file
Bool_t TPackage::AddFromFile(const char *fileName, const char *dest)
{
    if (gSystem -> AccessPathName(fileName, kReadPermission) == kTRUE) {
        Error("AddFromFile", "%s does not exist or cannot be accessed", fileName);
        return kFALSE;
    }
    std::string line;
    std::ifstream inFile(fileName);
    if (inFile.is_open()) {
        while (inFile.good()) {
            std::getline(inFile, line);
            if (line.size() > 0) {
                std::vector<std::string> tokens = split(line, ' ');
                if (dest) {
                    if (tokens.size() > 1) {
                        Warning("AddFromFile", "Ignoring parameter destination, using destination from file");
                        Add(tokens[0].c_str(), tokens[1].c_str());
                    }
                    else
                        Add(tokens[0].c_str(), dest);
                }
                else
                    Add(tokens[0].c_str(), dest);
            }
        }
        inFile.close();
    } else {
        Error("AddFromFile", "%s could not be opened", fileName);
        return kFALSE;
    }
    return kTRUE;
}

// add file to META_INF directory
Bool_t TPackage::AddToINF(const char *srcFile, const char *refStr, const char *funName)
{
    const char* fileName = gSystem -> BaseName(srcFile);
    TString strFileName = fileName;
    strFileName.ToUpper();
    if (strFileName != refStr) {
        Error(funName, "File must be called %s", refStr);
        return kFALSE;
    }
    if (gSystem -> AccessPathName(srcFile) == kTRUE) {
        Error(funName, "Could not access %s", srcFile);
        return kFALSE;
    }
    if (created == kTRUE) {
        Error(funName, "Package already created. Cannot add!");
        return kFALSE;
    }
    TString targetFile;
    if (gSystem -> IsAbsoluteFileName(srcFile))
        targetFile.Form("%s/%s", gSystem -> WorkingDirectory(), srcFile);
    else
        targetFile = srcFile;
    TString destination;
    destination.Form("%s/%s", tempDirContent.Data(), INF_DIR);
    CpWithList(targetFile.Data(), destination.Data(), packFiles);
    TString destFileName1, destFileName2;
    destFileName1.Form("%s/%s", destination.Data(), fileName);
    destFileName2.Form("%s/%s", destination.Data(), refStr);
    gSystem -> Rename(destFileName1.Data(), destFileName2.Data());
    TPackageFile *pFile = (TPackageFile *) packFiles -> FindObject(destFileName1.Data());
    pFile -> SetFilePath(destFileName2.Data());
    Info(funName, "Added %s to package", refStr);
    return kTRUE;
}

// add Build script to META_INF
Bool_t TPackage::AddBuild(const char *srcFile)
{
    Bool_t status = AddToINF(srcFile, BUILD_SCRIPT, "AddBuild");
    if (status == kTRUE)
        hasBuild = kTRUE;
    return status;
}

// add Setup macro to META_INF
Bool_t TPackage::AddSetup(const char *srcFile)
{
    Bool_t status = AddToINF(srcFile, SETUP_MACRO, "AddSetup");
    if (status == kTRUE)
        hasSetup = kTRUE;
    return status;
}

// add dependency
Bool_t TPackage::AddDep(const char *dep)
{
    TPackageRange *interval = TPackageRange::CreateRange(dep);
    if (interval == NULL)
        return kFALSE;
    deps -> Add(interval);
    Info("AddDep", "Added dependency %s", dep);
    return kTRUE;
}

// method to remove file from package
Bool_t TPackage::Remove(const char *srcFile)
{
    TString strFile = srcFile;
    if (created == kTRUE) {
        Error("Remove", "Package already created. Cannot remove!");
        return kFALSE;
    }
    if (strFile.Contains(INF_DIR)) {
        Error("Remove", "%s is a reserved directory", INF_DIR);
        return kFALSE;
    }
    if (gSystem -> IsAbsoluteFileName(srcFile)) {
        Error("Remove", "Cannot specify an absolute path. Path must be relative to main package directory");
        return kFALSE;
    }
    strFile.Form("%s/%s", tempDirContent.Data(), srcFile);
    TString path = strFile;
    if (path.Contains("*") && path.First('*') > path.Last('/')) {
        Int_t index = path.Last('/');
        path.Remove(index);
    }
    if (gSystem -> AccessPathName(path.Data(), kWritePermission) == kTRUE) {
        Error("Remove", "File %s does not exist", srcFile);
        return kFALSE;
    }
    RmWithList(strFile.Data(), kFALSE, packFiles);
    Info("Remove", "Removed %s from package", srcFile);
    return kTRUE;
}

// remove file from META_INF directory
Bool_t TPackage::RemoveFromINF(const char *refStr, const char *funName)
{
    if (created == kTRUE) {
        Error(funName, "Package already created. Cannot remove!");
        return kFALSE;
    }
    TString targetFile;
    targetFile.Form("%s/%s/%s", tempDirContent.Data(), INF_DIR, refStr);
    if (gSystem -> AccessPathName(targetFile.Data(), kWritePermission) == kTRUE) {
        Error(funName, "%s/%s does not exist", INF_DIR, refStr);
        return kFALSE;
    }
    RmWithList(targetFile.Data(), kTRUE, packFiles);
    Info(funName, "Removed %s from package", refStr);
    return kTRUE;
}

// remove Build script from META_INF
Bool_t TPackage::RemoveBuild()
{
    Bool_t status = RemoveFromINF(BUILD_SCRIPT, "RemoveBuild");
    if (status == kTRUE)
        hasBuild = kFALSE;
    return status;
}

// remove Setup macro from META_INF
Bool_t TPackage::RemoveSetup()
{
    Bool_t status = RemoveFromINF(SETUP_MACRO, "RemoveSetup");
    if (status == kTRUE)
        hasSetup = kFALSE;
    return status;
}

// remove dependency
Bool_t TPackage::RemoveDep(Int_t index)
{
    TObject *interval = deps -> At(index);
    if (interval == NULL) {
        Error("RemoveDep", "Invalid index");
        return kFALSE;
    }
    deps -> Remove(interval);
    return kTRUE;
}

// recursive method to store MD5 and lengths of all files in checksum file of META-INF
void TPackage::StoreFilesInfo(const char *dir, std::ofstream &outFile) const
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
            if (fname != "." && fname != ".." && fname != CHECKSUMS_FILE) {
                if (file -> IsDirectory() == kTRUE)
                    StoreFilesInfo(path.Data(), outFile);
                else {
                    TPackageFile *pFile = new TPackageFile(path);
                    pFile -> MakeRelativePath(contentLocation.Data());
                    outFile << pFile -> GetFileSize() << " md5:" << pFile -> GetMD5() << " " <<
                                pFile -> GetFilePath() << std::endl;
                    delete pFile;
                }
            }
        }
    }
}

// method to store MD5 and lengths of all files in checksum file of META-INF
void TPackage::StoreFilesInfo(std::ofstream &outFile) const
{
    TString currentDir = gSystem -> WorkingDirectory();
    gSystem -> ChangeDirectory(tempDirContent.Data());
    TIter next(packFiles);
    TPackageFile *pFile;
    while ((pFile = (TPackageFile*)next())) {
        outFile << pFile -> GetFileSize() << " md5:" << pFile -> GetMD5() << " " <<
            pFile -> GetFilePath() << std::endl;
    }
    gSystem -> ChangeDirectory(currentDir.Data());
}

// call to recursive method for storing MD5 and lengths of all files in checksum file of META-INF
void TPackage::StoreFilesInfo(Bool_t fromList) const
{
    TString content, path;
    if (tempDir.Length() == 0) return;
    if (created == kTRUE)
        content = contentLocation;
    else
        content = tempDirContent;
    path.Form("%s/%s/%s", content.Data(), INF_DIR, CHECKSUMS_FILE);
    ofstream infFile;
    infFile.open(path.Data());
    if (fromList == kTRUE)
        StoreFilesInfo(infFile);
    else
        StoreFilesInfo(content.Data(), infFile);
    infFile.close();
}

// store dependencies to file
void TPackage::StoreDependencies() const
{
    TString path;
    if (tempDir.Length() == 0) return;
    path.Form("%s/%s/%s", tempDirContent.Data(), INF_DIR, DEPS_FILE);
    ofstream depsFile;
    depsFile.open(path.Data());
    if (deps) {
        TIter next(deps);
        TPackageRange *interval;
        while ((interval = (TPackageRange*)next())) {
            depsFile << interval -> GetRange().Data() << std::endl;
        }
    }
    depsFile.close();
    packFiles -> Add(new TPackageFile(path.Data()));
}

// trim the paths of all files to be added so only the path relative to the package is present
void TPackage::TrimPaths(const char *baseDir) const
{
    TIter next(packFiles);
    TPackageFile *pFile;
    while ((pFile = (TPackageFile*)next())) {
        pFile -> MakeRelativePath(baseDir);
    }
}

// create archive (PAR file)
Bool_t TPackage::Create()
{
    if (created == kTRUE) {
        Error("Create", "Already created");
        return kFALSE;
    }
    StoreDependencies();
    TrimPaths(tempDirContent.Data());
    StoreFilesInfo();
    if (gSystem -> AccessPathName(packLocation.Data(), kWritePermission) == kTRUE) {
        Error("Create", "Cannot create par file at location %s", packLocation.Data());
        return kFALSE;
    }
    if (gSystem -> AccessPathName(parFilePath.Data()) == kFALSE) {
        Warning("Create", "A PAR file with the same name exists. Removing it");
        Rm(parFilePath.Data());
    }
    TString cmd;
    TString currentDir = gSystem -> WorkingDirectory();
    gSystem -> ChangeDirectory(tempDir.Data());
    gSystem -> ChangeDirectory("..");
    cmd.Form(ZIP_CMD, parFilePath.Data(), completeName.Data());
    if (gSystem -> Exec(cmd.Data())) {
        Error("Create", "%s did not execute successfully", cmd.Data());
        gSystem -> ChangeDirectory(currentDir.Data());
        return kFALSE;
    }
    gSystem -> ChangeDirectory(currentDir.Data());
    Info("Create", "PAR file %s%s successfully created at %s", completeName.Data(),
                    PAR_EXTENSION, packLocation.Data());
    Rm(tempDir.Data());
    created = kTRUE;
    return kTRUE;
}

// show package contents
void TPackage::ShowContents() const
{
    if (unpacked == kTRUE && gSystem -> AccessPathName(mainDirLocation.Data()) == kFALSE) {
        Ls(mainDirLocation.Data());
        return;
    }
    if (tempDir.Length() > 0 && gSystem -> AccessPathName(tempDir.Data()) == kFALSE) {
        Ls(tempDir.Data());
        return;
    }
    TString path;
    path.Form("%s/%s%s", packLocation.Data(), completeName.Data(), PAR_EXTENSION);
    if (gSystem -> AccessPathName(path.Data()) == kFALSE) {
        TString cmd;
        cmd.Form(LS_CMD2, path.Data());
        gSystem -> Exec(cmd.Data());
    }
}

// show class attribute values
void TPackage::ShowInfo() const
{
    std::cout << "Package name: " << packName.Data() << std::endl;
    std::cout << "Complete name: " << completeName.Data() << std::endl;
    std::cout << "Location of PAR file: " << parFilePath.Data() << std::endl;
    std::cout << "Location of package content: " << contentLocation.Data() << std::endl;
    std::cout << "Location of package binaries: " << binPath.Data() << std::endl;
    std::cout << "Package was created: " << (created ? "TRUE" : "FALSE") << std::endl;
    std::cout << "Package was unpacked: " << (unpacked ? "TRUE" : "FALSE") << std::endl;
}

// show checksums for files
void TPackage::ShowChecksums() const
{
    if (packFiles) {
        TIter next(packFiles);
        TPackageFile *pFile;
        while ((pFile = (TPackageFile*)next())) {
            std::cout << pFile -> GetFileSize() << " md5:" << pFile -> GetMD5() << " " <<
                pFile -> GetFileName() << " " << pFile -> GetFilePath() << std::endl;
        }
    }
}

// show dependencies
void TPackage::ShowDependencies() const
{
    if (deps) {
        TIter next(deps);
        TPackageRange *interval;
        while ((interval = (TPackageRange*)next())) {
            std::cout << interval -> GetRange().Data() << std::endl;
        }
    }
}

// show print options
void TPackage::ShowOptions() const
{
    std::cout << "Options:" << std::endl;
    std::cout << "\t info - show basic information about package" << std::endl;
    std::cout << "\t contents - show current package contents" << std::endl;
    std::cout << "\t checksums - show files checksums" << std::endl;
    std::cout << "\t deps - show dependencies" << std::endl;
    std::cout << "\t options - list print options" << std::endl;
    std::cout << "\t help - list print options" << std::endl;
}

// print method inherited from TObject
void TPackage::Print(Option_t *option) const
{
    TString opt = (const char *)option;
    if (opt == "info") {
        ShowInfo();
        return;
    }
    if (opt == "contents") {
        ShowContents();
        return;
    }
    if (opt == "checksums") {
        ShowChecksums();
        return;
    }
    if (opt == "deps") {
        ShowDependencies();
        return;
    }
    ShowOptions();
}

// method to restore the packFiles list after package was unpacked
void TPackage::RestorePackFiles()
{
    ClearList(packFiles);
    delete packFiles;
    packFiles = new TList();
    TString checksumsFile;
    checksumsFile.Form("%s/%s/%s", contentLocation.Data(), INF_DIR, CHECKSUMS_FILE);
    std::string line;
    std::ifstream inFile(checksumsFile.Data());
    if (inFile.is_open()) {
        while (inFile.good()) {
            std::getline(inFile, line);
            if (line.size() > 0) {
                std::vector<std::string> tokens = split(line, ' ');
                TString filePath;
                filePath.Form("%s/%s", contentLocation.Data(), tokens[tokens.size() - 1].c_str());
                if (gSystem -> AccessPathName(filePath.Data()) == kFALSE) {
                    TPackageFile *pFile = new TPackageFile(filePath.Data());
                    packFiles -> Add(pFile);
                }
            }
        }
    }
    inFile.close();
}

// method to restore the depdencies list
void TPackage::RestoreDeps()
{
    ClearList(deps);
    delete deps;
    deps = new TList();
    TString depsFile;
    depsFile.Form("%s/%s/%s", contentLocation.Data(), INF_DIR, DEPS_FILE);
    std::string line;
    std::ifstream inFile(depsFile.Data());
    if (inFile.is_open()) {
        while (inFile.good()) {
            std::getline(inFile, line);
            if (line.size() > 0) {
                TPackageRange *interval = new TPackageRange(line.c_str());
                if (interval -> TestBit(kInvalidObject) == kFALSE)
                    deps -> Add(interval);
            }
        }
    }
    inFile.close();
}



Bool_t TPackage::CheckPackStructure() const
{
    if (gSystem -> AccessPathName(contentLocation.Data()) == kTRUE) {
        Error("CheckPackStructure", "Could not find %s", contentLocation.Data());
        return kFALSE;
    }
    TString libDir;
    libDir.Form("%s/%s", mainDirLocation.Data(), LIB_DIR);
    if (gSystem -> AccessPathName(libDir.Data()) == kTRUE) {
        Error("CheckPackStructure", "Could not find %s", libDir.Data());
        return kFALSE;
    }
    TString infDir;
    infDir.Form("%s/%s", contentLocation.Data(), INF_DIR);
    if (gSystem -> AccessPathName(infDir.Data()) == kTRUE) {
        Error("CheckPackStructure", "Could not find %s", infDir.Data());
        return kFALSE;
    }
    TString checksumsFile;
    checksumsFile.Form("%s/%s", infDir.Data(), CHECKSUMS_FILE);
    if (gSystem -> AccessPathName(checksumsFile.Data()) == kTRUE) {
        Error("CheckPackStructure", "Could not find %s", checksumsFile.Data());
        return kFALSE;
    }
    return kTRUE;
}

// check package consistency
Bool_t TPackage::CheckConsistency() const
{
    if (CheckPackStructure() == kFALSE)
        return kFALSE;
    TString checksumsFile;
    checksumsFile.Form("%s/%s/%s", contentLocation.Data(), INF_DIR, CHECKSUMS_FILE);
    std::string line;
    std::ifstream inFile(checksumsFile.Data());
    Int_t numFiles = 0;
    if (inFile.is_open()) {
        while (inFile.good()) {
            std::getline(inFile, line);
            if (line.size() > 0) {
                numFiles++;
                std::vector<std::string> tokens = split(line, ' ');
                TString filePath;
                filePath.Form("%s/%s", contentLocation.Data(), tokens[tokens.size() - 1].c_str());
                if (gSystem -> AccessPathName(filePath.Data()) == kTRUE) {
                    inFile.close();
                    Info("CheckConsistency", "File %s does not exist", filePath.Data());
                    return kFALSE;
                }
                TPackageFile *pFile = new TPackageFile(filePath.Data());
                if (pFile -> GetFileSize() != atoi(tokens[0].c_str())) {
                    inFile.close();
                    Info("CheckConsistency", "File %s has invalid size", filePath.Data());
                    return kFALSE;
                }
                TString newLine;
                newLine.Form("md5:%s", pFile -> GetMD5());
                if (newLine != tokens[1].c_str()) {
                    inFile.close();
                    Info("CheckConsistency", "File %s does have correct checksum", filePath.Data());
                    return kFALSE;
                }
            }
        }
    }
    inFile.close();
    if (numFiles != GetNumFiles(contentLocation.Data()) - 1) {
        Info("CheckConsistency", "Number of files does not match");
        return kFALSE;
    }
    return kTRUE;
}

// unzip package
Bool_t TPackage::Unpack()
{
    if (parFilePath.Length() == 0 || gSystem -> AccessPathName(parFilePath.Data(), kReadPermission)) {
        Error("Unpack", "par file does not exists");
        return kFALSE;
    }
    if (gSystem -> AccessPathName(mainDirLocation.Data()) == kFALSE)
        Rm(mainDirLocation.Data());
    TString cmd;
    cmd.Form(UNZIP_CMD, parFilePath.Data(), allPacksLocation.Data());
    if (gSystem -> Exec(cmd.Data())) {
        Error("Unpack", "failure executing: %s", cmd.Data());
        return kFALSE;
    }
    if (CheckConsistency() == kFALSE) {
        if (strictCheck == kTRUE) {
            Error("Unpack", "Package is not consistent");
            SetBit(kInvalidObject);
            Rm(mainDirLocation.Data());
            return kFALSE;
        } else {
            Warning("Unpack", "Package is not consistent");
            TString checksumsFile;
            checksumsFile.Form("%s/%s/%s", contentLocation.Data(), INF_DIR, CHECKSUMS_FILE);
            Rm(checksumsFile.Data());
            StoreFilesInfo(kFALSE);
        }
    }
    Info("Unpack", "par file %s successfully unpacked", parFilePath.Data());
    created = kTRUE;
    unpacked = kTRUE;
    RestorePackFiles();
    RestoreDeps();
    TString buildLocation, setupLocation;
    buildLocation.Form("%s/%s/%s", contentLocation.Data(), INF_DIR, BUILD_SCRIPT);
    setupLocation.Form("%s/%s/%s", contentLocation.Data(), INF_DIR, SETUP_MACRO);
    if (gSystem -> AccessPathName(buildLocation.Data()) == kFALSE)
        hasBuild = kTRUE;
    else
        hasSetup = kFALSE;
    if (gSystem -> AccessPathName(setupLocation.Data()) == kFALSE)
        hasSetup = kTRUE;
    else
        hasSetup = kFALSE;

    gPackMan -> Add(this);

    return kTRUE;
}

// check if package was built for current configuration (architecture, version etc)
Bool_t TPackage::HasBuilt() const
{
    TString path;
    path.Form("%s/%s.%s", binPath.Data(), sharedLibName.Data(), gSystem -> GetSoExt());
    if (gSystem -> AccessPathName(path.Data()) == kTRUE)
        return kFALSE;
    else
        return kTRUE;
}

// execute build script from package
Bool_t TPackage::BuildFromScript(TString location) const
{
    Bool_t status = kTRUE;
    TString currentDir = gSystem -> WorkingDirectory();
    gSystem -> ChangeDirectory(location.Data());
    TString cmd;
    cmd.Form("export ROOTPROOFCLIENT=\"1\" ; %s/%s", INF_DIR, BUILD_SCRIPT);
    if (gSystem->Exec(cmd.Data())) {
        Error("Build", "building package %s failed", completeName.Data());
        status = kFALSE;
    }
    gSystem -> ChangeDirectory(currentDir.Data());
    return status;
}

// method for determining which source files are used when compiling
TString TPackage::GetSourceFiles() const
{
    TString srcFiles = "";
    if (packFiles) {
        TIter next(packFiles);
        TPackageFile *pFile;
        while ((pFile = (TPackageFile*)next())) {
            TString fileName = pFile -> GetFileName();
            if (fileName.EndsWith(".cxx") || fileName.EndsWith(".cpp") || fileName.EndsWith(".cc")) {
                srcFiles += " ";
                srcFiles += pFile -> GetFilePath();
            }
        }
    }
    return srcFiles;
}

// method for determining which header files are used when compiling
TString TPackage::GetHeaderFiles() const
{
    TString hFiles = "";
    TString last;
    if (packFiles) {
        TIter next(packFiles);
        TPackageFile *pFile;
        while ((pFile = (TPackageFile*)next())) {
            TString fileName = pFile -> GetFileName();
            if (fileName.EndsWith(".h")) {
                if (fileName.EndsWith("LinkDef.h"))
                    last = pFile -> GetFilePath();
                else {
                    hFiles += " ";
                    hFiles += pFile -> GetFilePath();
                }
            }
        }
    }
    if (last.Length() > 0) {
        hFiles += " ";
        hFiles += last;
    }
    return hFiles;
}

// method for determining what object files will be generated after compile
TString TPackage::GetObjectFiles() const
{
    TString objFiles = "";
    TString objExt, metaSetup;
    objExt.Form(".%s", gSystem -> GetObjExt());
    metaSetup.Form("%s/%s", INF_DIR, SETUP_MACRO);
    if (packFiles) {
        TIter next(packFiles);
        TPackageFile *pFile;
        while ((pFile = (TPackageFile*)next())) {
            TString fileName = pFile -> GetFileName();
            if (IsSourceFile(fileName) && fileName != metaSetup) {
                RemoveExtension(fileName);
                fileName += objExt;
                objFiles += " ";
                objFiles += fileName.Data();
            }
        }
    }
    return objFiles;
}

// build files in the order in which they were added
Bool_t TPackage::BuildStandard(TString location) const
{
    TString srcFiles = GetSourceFiles();
    if (srcFiles.Length() == 0) {
        Error("BuildStandard", "No source files detected");
        return kFALSE;
    }
    TString objFiles = GetObjectFiles();
    TString makeSharedLib = gSystem -> GetMakeSharedLib();
    makeSharedLib.ReplaceAll("-Wl,--no-undefined", "");
    TString includePath = gSystem -> GetIncludePath();
    TString linkedLibs = gSystem -> GetLinkedLibs();
    TString sharedLib;
    sharedLib.Form("%s.so", sharedLibName.Data());
    TString envVariables;
    TString currentDirectory = gSystem -> WorkingDirectory();
    gSystem -> ChangeDirectory(location.Data());
    envVariables.Form("export BuildDir=. ; export IncludePath=\"%s\" ; export SourceFiles=\"%s\" ; \
                        export ObjectFiles=\"%s\" ; export LibName=%s ; export LinkedLibs=\"%s\" ; \
                        export  SharedLib=%s", includePath.Data(), srcFiles.Data(),
                        objFiles.Data(), sharedLibName.Data(), linkedLibs.Data(), sharedLib.Data());
    TString buildCmd;
    buildCmd.Form("%s ; %s", envVariables.Data(), makeSharedLib.Data());

    if (gSystem -> Exec(buildCmd.Data()) != 0) {
        Error("BuildStandard", "Could not build");
        gSystem -> ChangeDirectory(currentDirectory.Data());
        return kFALSE;
    }
    gSystem -> ChangeDirectory(currentDirectory.Data());
    return kTRUE;
}


// make directory for storing binary files
TString *TPackage::MakeBinariesDir(TString location)
{
    TString odbg;
    TString *binLocation = new TString();
    if (TString(ROOTBUILD) == TString("debug"))
        odbg.Form("-dbg");
    TString binDir1, binDir2, binDir3;
    binDir1.Form("%s/%s", location.Data(), LIB_DIR);
    binDir2.Form("%s/%s/ROOT-%d", location.Data(), LIB_DIR, gROOT -> GetVersionInt());
    binDir3.Form("%s/%s/ROOT-%d/%s-%s%s", location.Data(), LIB_DIR, gROOT -> GetVersionInt(),
            gSystem -> GetBuildArch(), gSystem -> GetBuildCompilerVersion(), odbg.Data());
    gSystem -> MakeDirectory(binDir1.Data());
    gSystem -> MakeDirectory(binDir2.Data());
    gSystem -> MakeDirectory(binDir3.Data());
    if (gSystem -> AccessPathName(binDir3.Data()) == kFALSE) {
        binLocation -> Form("%s", binDir3.Data());
        return binLocation;
    }  else {
        Error("MakeBinariesDir", "Cannot create %s", binDir3.Data());
        return NULL;
    }
}

// move binary files to special purpose directory
void TPackage::MoveBinaryFiles(TString location, TString binLocation) const
{
    TString currentDir = gSystem -> WorkingDirectory();
    gSystem -> ChangeDirectory(location.Data());
    TString objectFiles;
    objectFiles.Form("%s/%s", location.Data(), OBJ_EXTENSION);
    Rm(objectFiles.Data());
    TList *files = GetFiles(location.Data());
    if (files) {
        TSystemFile *file;
        TString fname;
        TIter next(files);
        while ((file = (TSystemFile*)next())) {
            fname = file -> GetName();
            TString path;
            path.Form("%s/%s", location.Data(), fname.Data());
            if (fname != "." && fname != "..") {
                if (gSystem -> AccessPathName(path.Data(), kExecutePermission) == kFALSE &&
                    file -> IsDirectory() == kFALSE) {
                    Cp(path.Data(), binLocation.Data());
                    Rm(path.Data());
                }
            }
        }
    }
    gSystem -> ChangeDirectory(currentDir.Data());
}

// execute build script from package
Bool_t TPackage::Build()
{
    Bool_t status = kFALSE;
    TString mainDir, content;
    if (created == kFALSE) {
        mainDir = tempDir;
        content = tempDirContent;
    }
    else
        if (unpacked == kFALSE) {
            Error("Build", "Package is archived. Unpack first");
            return kFALSE;
        } else {
            mainDir = mainDirLocation;
            content = contentLocation;
        }
    if (hasBuild == kTRUE)
        status = BuildFromScript(content);
    else
        status = BuildStandard(content);
    if (status == kFALSE) {
        Error("Build", "Could not build");
        return kFALSE;
    }
    TString *binLocation = MakeBinariesDir(mainDir);
    if (binLocation) {
        MoveBinaryFiles(content, *binLocation);
        status = kTRUE;
    } else
        status = kFALSE;
    delete binLocation;
    if (status == kTRUE)
        Info("Build", "Build successful");
    return status;
}

// create package object from a given PAR file
TPackage *TPackage::OpenPAR(const char *parFile, Bool_t check)
{
    if (gSystem -> AccessPathName(parFile, kReadPermission) == kTRUE) {
        std::cout << "File " << parFile << " does not exist or cannot be read" << std::endl;
        return NULL;
    }
    TString parFileLocation = parFile;
    RemoveExtension(parFileLocation);
    TString parName = gSystem -> BaseName(parFileLocation); // name of the par file
    parFileLocation.Remove(parFileLocation.Length() - parName.Length());
    TString parPath; // absolute path to containing directory
    if (gSystem -> IsAbsoluteFileName(parFile))
        parPath = parFileLocation;
    else
        parPath.Form("%s/%s", gSystem -> WorkingDirectory(), parFileLocation.Data());
    if (parPath.EndsWith("/"))
        parPath.Remove(parPath.Length() - 1);
    Int_t majVers = -1, minVers = -1;
    TString leftPart, rightPart;
    if (parName.Contains("-") == kFALSE) {
        std::cout << "Invalid file name" << std::endl;
        return NULL;
    }
    leftPart = parName(0, parName.Index("-"));
    rightPart = parName(parName.Index("-") + 1, parName.Length() - 1);
    char patch[parName.Length()];
    memset(patch, 0, parName.Length());
    if (sscanf(rightPart.Data(), "%d.%d.%s", &majVers, &minVers, patch) < 3)
        if (sscanf(rightPart.Data(), "%d.%d", &majVers, &minVers) < 2) {
            std::cout << "Invalid file name." << std::endl;
            return NULL;
        }
    TPackage *p = new TPackage(leftPart.Data(), majVers, minVers, patch, check, parPath.Data(), 1);
    return p;
}

// create package object from a given package directory
TPackage *TPackage::OpenDir(const char *dir, Bool_t check)
{
    TString path = NormalizePath(dir);
    TString currentDir = gSystem -> WorkingDirectory();
    if (path.Length() == 0)
        return NULL;
    TString content = path;
    TString name = gSystem -> BaseName(path.Data());
    path.Remove(path.Length() - name.Length() - 1);
    if (path.EndsWith(PACK_DIR) == kFALSE)
        return NULL;
    TString packDir = PACK_DIR;
    path.Remove(path.Length() - packDir.Length());
    gSystem -> ChangeDirectory(path.Data());
    TPackage *p = new TPackage(name.Data(), check, (const char *)NULL, 2);
    gSystem -> ChangeDirectory(currentDir.Data());
    return p;
}

// create package object from a PAR file or package directory
TPackage *TPackage::Open(const char *packPath, Bool_t check)
{
    TString strPackPath = packPath;
    TPackage *p;
    if (strPackPath.EndsWith(PAR_EXTENSION))
        p = OpenPAR(packPath, check);
    else
        p = OpenDir(packPath, check);
    if (p == NULL || p -> TestBit(kInvalidObject)) {
        if (p) delete p;
        return NULL;
    }
    if (strPackPath.EndsWith(PAR_EXTENSION) == kFALSE)
        gPackMan -> Add(p);

    return p;
}

// save setup macro
static TString SaveSetup(TString setup, TString setupfn, TString pathToSetup)
{
    TMacro setupmc(pathToSetup.Data());
    TObjString *setupline = setupmc.GetLineWith("SETUP(");
    if (setupline) {
        TString setupstring(setupline -> GetString());
        setupstring.ReplaceAll("SETUP(", TString::Format("%s(", setup.Data()));
        setupline->SetString(setupstring);
    } else {
        // Macro does not contain SETUP()
        Warning("SaveSetup", "macro '%s/%s' does not contain a SETUP()"
                " function", gSystem -> WorkingDirectory(), pathToSetup.Data());
    }
    setupmc.SaveSource(setupfn.Data());
    return setupfn;
}

// create call environment for calling the setup macro and return it
static TMethodCall *GetCallEnv(TList *loadopts, TFunction *fun, TString setup)
{
    TMethodCall *callEnv = new TMethodCall();
    // Check the number of arguments
    switch(fun -> GetNargs()) {
        case 0: {
                    callEnv -> InitWithPrototype(setup, ""); // No arguments (basic signature)
                    if (loadopts) // Warn that the argument (if any) if ignored
                        Warning("GetCallEnv", "loaded SETUP() does not take any argument:"
                                " the specified TList object will be ignored");
                    break;
                }
        case 1: {
                    TMethodArg *arg = (TMethodArg *) fun -> GetListOfMethodArgs() -> First();
                    if (!arg) {
                        Error("GetCallEnv", "cannot get information about the SETUP() argument:"
                                " cannot continue");
                        return NULL;
                    }
                    // Check argument type
                    TString argsig(arg -> GetTitle());
                    if (argsig.BeginsWith("TList")) {
                        callEnv -> InitWithPrototype(setup,"TList *");
                        callEnv -> ResetParam();
                        callEnv -> SetParam((Long_t) loadopts);
                    } else if (argsig.BeginsWith("const char")) {
                        callEnv -> InitWithPrototype(setup,"const char *");
                        callEnv -> ResetParam();
                        TObjString *os = loadopts ? dynamic_cast<TObjString *>(loadopts->First()) : 0;
                        if (os) {
                            callEnv -> SetParam((Long_t) os->GetName());
                        } else {
                            if (loadopts && loadopts->First()) {
                                Warning("GetCallEnv", "found object argument of type %s:"
                                        " SETUP expects 'const char *': ignoring",
                                        loadopts->First()->ClassName());
                            }
                            callEnv -> SetParam((Long_t) 0);
                        }
                    } else {
                        Error("GetCallEnv", "unsupported SETUP signature: SETUP(%s)"
                                " cannot continue", arg->GetTitle());
                        return NULL;
                    }
                    break;
                }
        default: {
                    Error("GetCallEnv", "function SETUP() can have at most a 'TList *' argument:"
                            " cannot continue");
                    return NULL;
                }
    }
    return callEnv;
}

// Remove temporary macro file and change back to current directory
static void Restore(TString currentDir, TString setupfn)
{
    if (!gSystem->AccessPathName(setupfn.Data())) gSystem->Unlink(setupfn.Data());
    gSystem->ChangeDirectory(currentDir.Data());
}

// run setup macro
Bool_t TPackage::RunSetup(TList *loadopts) const
{
    TString pathToSetup;
    pathToSetup.Form("%s/%s/%s", contentLocation.Data(), INF_DIR, SETUP_MACRO);
    TString currentDir = gSystem -> WorkingDirectory();
    gSystem -> ChangeDirectory(binPath.Data());

    // We need to change the name of the function to avoid problems when we load more packages
    TString setup, setupfn;
    setup.Form("SETUP_%x", TString(completeName.Data()).Hash());
    setupfn.Form("%s/%s.C", gSystem -> TempDirectory(), setup.Data());
    SaveSetup(setup, setupfn, pathToSetup);

    // Could not load macro
    if (gROOT -> LoadMacro(setupfn.Data()) != 0) {
        Error("Load", "macro '%s/%s' could not be loaded:"
                " cannot continue", contentLocation.Data(), pathToSetup.Data());
        Restore(currentDir, setupfn);
        return kFALSE;
    }

    // Check the signature
    TFunction *fun = (TFunction *) gROOT->GetListOfGlobalFunctions()->FindObject(setup);
    if (!fun) {
        Error("Load", "function SETUP() not found in macro '%s/%s':"
                " cannot continue", contentLocation.Data(), pathToSetup.Data());
        Restore(currentDir, setupfn);
        return kFALSE;
    }

    TMethodCall *callEnv = GetCallEnv(loadopts, fun, setup);
    Long_t setuprc = 0;
    if (callEnv == NULL) {
        callEnv = new TMethodCall();
        setuprc = -1;
    }
    callEnv -> Execute(setuprc);
    if (setuprc < 0) {
        Restore(currentDir, setupfn);
        return kFALSE;
    }
    Restore(currentDir, setupfn);
    return kTRUE;
}

// load/unload shared objects from default binaries location
Bool_t TPackage::LoadUnloadLibs(Bool_t load) const
{
    TList *files = GetFiles(binPath.Data());
    TString soExt;
    soExt.Form(".%s", gSystem -> GetSoExt());
    TString currentDir = gSystem -> WorkingDirectory();
    gSystem -> ChangeDirectory(binPath.Data());
    const char *msg1 = (load == kTRUE ? "Loaded" : "Unloaded");
    const char *msg2 = (load == kTRUE ? "load" : "unload");
    Int_t callResult = -1;
    if (files) {
        TSystemFile *file;
        TString fname;
        TIter next(files);
        while ((file = (TSystemFile*)next())) {
            fname = file -> GetName();
            if (!file -> IsDirectory() && fname.EndsWith(soExt)) {
                if (load == kTRUE)
                    callResult = gSystem -> Load(fname.Data());
                else {
                    gSystem -> Unload(fname.Data());
                    callResult = 0;
                }
                if (callResult == 0)
                    Info("LoadUnloadLibs", "%s %s", msg1, fname.Data());
                else {
                    Error("LoadUnloadLibs", "Could not %s %s", msg2, fname.Data());
                    gSystem -> ChangeDirectory(currentDir.Data());
                    return kFALSE;
                }
            }
        }
    }
    gSystem -> ChangeDirectory(currentDir.Data());
    return kTRUE;
}

// check if package is already loaded
Bool_t TPackage::IsLoaded() const
{
    TList *files = GetFiles(binPath.Data());
    TString soExt, loadedLibs;
    soExt.Form(".%s", gSystem -> GetSoExt());
    loadedLibs = gSystem -> GetLibraries();
    TString currentDir = gSystem -> WorkingDirectory();
    gSystem -> ChangeDirectory(binPath.Data());
    if (files) {
        TSystemFile *file;
        TString fname;
        TIter next(files);
        while ((file = (TSystemFile*)next())) {
            fname = file -> GetName();
            if (!file -> IsDirectory() && fname.EndsWith(soExt)) {
                if (loadedLibs.Contains(fname)) {
                    gSystem -> ChangeDirectory(currentDir.Data());
                    return kTRUE;
                }
                else {
                    gSystem -> ChangeDirectory(currentDir.Data());
                    return kFALSE;
                }
            }
       }
    }
    gSystem -> ChangeDirectory(currentDir.Data());
    return kFALSE;
}

// load package
Bool_t TPackage::Load(TList *loadopts, Bool_t recompile)
{
    if (unpacked == kFALSE) {
        Error("Load", "Package was not unpacked");
        return kFALSE;
    }
    if (IsLoaded() == kTRUE) {
        Info("Load", "Package is already loaded. Doing nothing");
        return kTRUE;
    }
    if (deps -> GetSize() > 0) {
        if (gPackMan -> ResolveDeps(this) == kFALSE) {
            Error("Load", "Could not resolve dependencies");
            return kFALSE;
        }
    }
    if (recompile == kTRUE && Build() == kFALSE) {
        Error("Load", "Could not recompile");
        return kFALSE;
    }
    if (hasSetup == kFALSE) {
        Info("Load", "package %s exists but has no %s script", completeName.Data(), SETUP_MACRO);
        if (LoadUnloadLibs(kTRUE) == kFALSE) {
            Info("Load", "Generating dict files and trying again");
            if (GenerateLinkDef() == kTRUE && GenerateDict() == kTRUE) {
                Clean();
                if (Build() == kFALSE || LoadUnloadLibs(kTRUE) == kFALSE) {
                    Error("Load", "Dict files did not help. Package could not be loaded");
                    ClearDictFiles();
                    return kFALSE;
                } else
                    StoreFilesInfo();
            } else {
                Error("Load", "Could not generate dict files");
                return kFALSE;
            }
        }
    }
    else
        if (RunSetup(loadopts) == kFALSE)
            return kFALSE;

    // Add package directory to list of include directories to be searched by ACliC
    gSystem -> AddIncludePath(TString("-I") + contentLocation.Data());

    // add package directory to list of include directories to be searched by CINT
    gROOT -> ProcessLine(TString(".include ") + contentLocation.Data());

    Info("Load", "package %s successfully loaded", completeName.Data());

    gPackMan -> Add(this);

    return kTRUE;
}

// unload package
Bool_t TPackage::Unload() const
{
    if (IsLoaded() == kTRUE) {
        Warning("Unload", "Package is not loaded. Doing nothing");
        return kTRUE;
    }
    if (LoadUnloadLibs(kFALSE) == kFALSE) {
        Error("Unload", "Could not unload package");
        return kFALSE;
    }
    Info("Unload", "Package successfully unloaded");
    return kTRUE;
}

// generate LinkDef file
Bool_t TPackage::GenerateLinkDef() const
{
    TString path;
    path.Form("%s/%sLinkDef.h", contentLocation.Data(), packName.Data());
    if (gSystem -> AccessPathName(path.Data()) == kFALSE)
        return kTRUE;
    TString headers = GetHeaderFiles();
    if (headers.Contains("LinkDef.h"))
        return kTRUE;
    ofstream linkDefFile;
    linkDefFile.open(path.Data());
    linkDefFile << "#ifdef __CINT__" << std::endl;
    linkDefFile << "#pragma link off all globals;" << std::endl;
    linkDefFile << "#pragma link off all classes;" << std::endl;
    linkDefFile << "#pragma link off all functions;" << std::endl;
    if (packFiles) {
        TIter next(packFiles);
        TPackageFile *pFile;
        while ((pFile = (TPackageFile*)next())) {
            TString fileName = pFile -> GetFileName();
            if (fileName.EndsWith(".h")) {
                RemoveExtension(fileName);
                linkDefFile << "#pragma link C++ class " << fileName.Data() << "+;" << std::endl;
            }
        }
    }
    linkDefFile << "#endif" << std::endl;
    linkDefFile.close();
    packFiles -> Add(new TPackageFile(path.Data()));
    return kTRUE;
}

// generate dictionary files
Bool_t TPackage::GenerateDict() const
{
    TString cmd, path, currentDir;
    currentDir = gSystem -> WorkingDirectory();
    gSystem -> ChangeDirectory(contentLocation.Data());
    path.Form("%s/%sLinkDef.h", contentLocation.Data(), packName.Data());
    cmd.Form("rootcint -f %sDict.cxx -c %s", packName.Data(), GetHeaderFiles().Data());
    if (gSystem -> Exec(cmd.Data()) != 0) {
        Error("GenerateDict", "Could not generate dictionary files");
        if (gSystem -> AccessPathName(path.Data()) == kFALSE)
            Rm(path.Data());
        gSystem -> ChangeDirectory(currentDir.Data());
        return kFALSE;
    }
    TString dictCxx, dictH;
    dictCxx.Form("%s/%sDict.cxx", contentLocation.Data(), packName.Data());
    dictH.Form("%s/%sDict.h", contentLocation.Data(), packName.Data());
    packFiles -> Add(new TPackageFile(dictCxx.Data()));
    packFiles -> Add(new TPackageFile(dictH.Data()));
    gSystem -> ChangeDirectory(currentDir.Data());
    return kTRUE;
}

// remove linkdef and dict files
void TPackage::ClearDictFiles() const
{
    TString linkDef, dictCxx, dictH;
    linkDef.Form("%s/%sLinkDef.h", contentLocation.Data(), packName.Data());
    dictCxx.Form("%s/%sDict.cxx", contentLocation.Data(), packName.Data());
    dictH.Form("%s/%sDict.h", contentLocation.Data(), packName.Data());
    Rm(linkDef.Data());
    Rm(dictCxx.Data());
    Rm(dictH.Data());
}

// generate makefile
Bool_t TPackage::GenerateMakefile() const
{
/*
    TString path;
    path.Form("%s/Makefile", contentLocation.Data());
    if (gSystem -> AccessPathName(path.Data()) == kFALSE) {
        Error("GenerateMakefile", "Makefile already exists");
        return kFALSE;
    }
    ofstream makefile;
    makefile.open(path.Data());

    makefile.close();
    packFiles -> Add(new TPackageFile(path.Data()));
*/
    return kTRUE;
}

// generate makefile and build script
Bool_t TPackage::GenerateBuild() const
{
/*
    if (created == kTRUE) {
        Error("GenerateBuild", "Package already created");
        return kFALSE;
    }
    TString path;
    path.Form("%s/%s/%s", contentLocation.Data(), INF_DIR, BUILD_SCRIPT);
    if (gSystem -> AccessPathName(path.Data()) == kFALSE) {
        Error("GenerateBuild", "Build already exists");
        return kFALSE;
    }
    if (GenerateMakefile() == kFALSE)
        return kFALSE;
    ofstream buildScript;
    buildScript.open(path.Data());
    buildScript << "make" << std::endl;
    buildScript.close();
    packFiles -> Add(new TPackageFile(path.Data()));
*/
    return kTRUE;
}

// enable package (unpack, build, load)
Bool_t TPackage::Enable(TList *loadopts, Bool_t recompile)
{
    if (created == kFALSE) {
        Error("Enable", "Package not created");
        return kFALSE;
    }
    if (unpacked == kFALSE && Unpack() == kFALSE) {
        Error("Enable", "Package could not be unpacked");
        return kFALSE;
    }
    if (HasBuilt() == kFALSE || recompile == kTRUE)
        if (Build() == kFALSE) {
            Error("Enable", "Package could not be built");
            return kFALSE;
        }
    if (Load(loadopts) == kFALSE) {
        Error("Enable", "Package could not be loaded");
        return kFALSE;
    }
    return kTRUE;
}

// clean current binaries directory
void TPackage::Clean() const
{
    TString path;
    path.Form("%s/*", binPath.Data());
    Rm(path.Data());
}

// clean everything in lib
void TPackage::CleanAll() const
{
    TString path;
    path.Form("%s/%s/*", mainDirLocation.Data(), LIB_DIR);
    Rm(path.Data());
}
