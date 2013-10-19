#include "TPackageManagerMeta.h"

TPackageManagerMeta *gPackMan;

// empty constructor
TPackageManagerMeta::TPackageManagerMeta()
{
   enabledPacks = new TList();
   packMans = new TList();
   reposMap = new TMap();
}

// method for adding packages
Int_t TPackageManagerMeta::Add(TPackage *pack)
{
   if (pack == NULL || pack -> TestBit(kInvalidObject)) {
      Error("Add", "Invalid package");
      return -1;
   }
   if (enabledPacks -> FindObject(pack -> GetName())) {
      Error("Add", "Package already added");
      return -2;
   }
   if (pack -> IsLoaded() == kTRUE)
      enabledPacks -> Add(pack);
   TObjString *packDir = new TObjString(pack -> GetPacksDir());
   TVirtualPackageManager *localManager = (TVirtualPackageManager *)
      (reposMap -> GetValue((TObject *)packDir));
   if (localManager == NULL) {
      localManager = new TPackageManagerDir(pack -> GetPacksDir());
      reposMap -> Add((TObject *)packDir, (TObject *)localManager);
      packMans -> Add(localManager);
      Info("Add", "Added package %s and created repository for %s", pack -> GetName(),
            pack -> GetPacksDir());
   } else {
      Info("Add", "Package added %s", pack -> GetName());
      delete packDir;
   }
   return 0;
}

// method for adding package managers to this manager
Int_t TPackageManagerMeta::Add(TVirtualPackageManager *packMan)
{
   if (packMan == NULL || packMan -> TestBit(kInvalidObject)) {
      Error("Add", "Invalid package manager");
      return -4;
   }
   if (packMans -> FindObject(packMan -> GetName())) {
      Error("Add", "Package manager already in list");
      return -5;
   }
   packMans -> Add((TObject *)packMan);
   reposMap -> Add((TObject *)(new TObjString(packMan -> GetName())), (TObject *)packMan);
   return 0;
}

// get package manager associated with a certain directory (repository)
TVirtualPackageManager *TPackageManagerMeta::GetRepo(const char *dir)
{
   TObjString *strDir = new TObjString(NormalizePath(dir).Data());
   if ((strDir -> GetString()).Length() == 0) {
      Error("GetRepo", "Invalid path");
      return NULL;
   }
   TVirtualPackageManager *localManager = (TVirtualPackageManager *)
      (reposMap -> GetValue((TObject *)strDir));
   if (localManager == NULL)
      Warning("GetRepo", "Repo does not exist");
   delete strDir;
   return localManager;
}

// get repository associated with a specific package
TVirtualPackageManager *TPackageManagerMeta::GetRepo(TPackage *pack)
{
   return GetRepo(pack -> GetPacksDir());
}

// locate a package object
TPackage *TPackageManagerMeta::LocatePackage(const char *packName)
{
   TPackage *package = (TPackage *)(enabledPacks -> FindObject(packName));
   if (package)
      return package;
   TIter next(packMans);
   TVirtualPackageManager *currentPackMan;
   while ((currentPackMan = (TVirtualPackageManager *)next())) {
      TList *packList = currentPackMan -> List();
      package = (TPackage *)packList -> FindObject(packName);
      if (package)
         return package;
   }
   return NULL;
}

// enable a specific package
Int_t TPackageManagerMeta::Enable(const char *packName)
{
   if (enabledPacks -> FindObject(packName)) {
      Info("Enable", "Package already enabled");
      return -1;
   }
   TPackage *package = LocatePackage(packName);
   if (package == NULL) {
      Error("Enable", "Could not locate package");
      return -2;
   }
   if (package -> Enable() == kFALSE) {
      Error("Enable", "Package %s could not be enabled", packName);
      return -3;
   }
   Info("Enable", "Package %s successfully enabled", packName);
   return 0;
}

// unload a certain enabled package
Int_t TPackageManagerMeta::Disable(const char *packName)
{
   TPackage *package = (TPackage *) enabledPacks -> FindObject(packName);
   if (package) {
      if (package -> Unload() == kFALSE) {
         Error("Disable", "Could not unload package");
         return -1;
      }
   }
   else {
      Error("Disable", "Package is not enabled");
      return -2;
   }
   Info("Disable", "Package %s successfully unloaded", packName);
   return 0;
}

// remove package from disk
Int_t TPackageManagerMeta::Remove(const char *packName)
{
   TPackage *package = (TPackage *) enabledPacks -> FindObject(packName);
   if (package) {
      if (Disable(packName) == -1) {
         Error("Remove", "Cannot remove package. Package could not be disabled");
         return -1;
      }
   } else
      package = LocatePackage(packName);
   if (package == NULL) {
      Error("Remove", "Package not found");
      return -2;
   }
   const char *packPath = package -> GetPath();
   Rm(packPath);
   Info("Remove", "Package successfully removed");
   return 0;
}

Int_t TPackageManagerMeta::Put()
{
   return 0;
}

Int_t TPackageManagerMeta::Get()
{
   return 0;
}

// return list of enabled packages
TList *TPackageManagerMeta::List()
{
   return enabledPacks;
}

// method for resolving dependency issues
Bool_t TPackageManagerMeta::ResolveDeps(TPackage *package)
{
   TList *deps = package -> GetDeps();
   TIter next1(packMans);
   TVirtualPackageManager *currentPackMan;
   while ((currentPackMan = (TVirtualPackageManager *)next1())) {
      TList *packList = currentPackMan -> List();
      TIter next2(packList);
      TPackage *depPack;
      while ((depPack = (TPackage *)next2())) {
         TPackageVersion *version = depPack -> GetVersion();
         TIter next3(deps);
         TPackageRange *range;
         Bool_t found = kFALSE;
         while ((range = (TPackageRange *)next3())) {
            if (range -> ContainsVersion(version)) {
               found = kTRUE;
               break;
            }
         }
         if (found == kTRUE) {
            if (enabledPacks -> FindObject(depPack -> GetName()) != NULL ||
                  Enable(depPack -> GetName()) == 0) {
               deps -> Remove(range);
            }
         }
         if (deps -> GetSize() == 0)
            return kTRUE;
      }
   }
   return kFALSE;
}

// show print options
void TPackageManagerMeta::ShowOptions() const
{
   std::cout << "Options:" << std::endl;
   std::cout << "\t enabled - show enabled packages" << std::endl;
   std::cout << "\t available - show available packages" << std::endl;
   std::cout << "\t packmans - show package managers" << std::endl;
   std::cout << "\t options - list print options" << std::endl;
   std::cout << "\t help - list print options" << std::endl;
}

// show enabled packages
void TPackageManagerMeta::ShowEnabledPackages() const
{
   TIter next(enabledPacks);
   TPackage *package;
   while ((package = (TPackage *)next()))
      std::cout << package -> GetName() << std::endl;
}

// show available packages
void TPackageManagerMeta::ShowAvailablePackages() const
{
   TIter next(packMans);
   TVirtualPackageManager *packMan;
   while ((packMan = (TVirtualPackageManager *)next())) {
      std::cout << packMan -> GetName() << std::endl;
      packMan -> Print("packages");
      std::cout << std::endl << std::endl;
   }
}

// show known package managers
void TPackageManagerMeta::ShowPackageManagers() const
{
   TIter next(packMans);
   TVirtualPackageManager *packMan;
   while ((packMan = (TVirtualPackageManager *)next()))
      std::cout << packMan -> GetName() << std::endl;
}

// print method
void TPackageManagerMeta::Print(Option_t *option) const
{
   TString opt = (const char *)option;
   if (opt == "enabled") {
      ShowEnabledPackages();
      return;
   }
   if (opt == "available") {
      ShowAvailablePackages();
      return;
   }
   if (opt == "packmans") {
      ShowPackageManagers();
      return;
   }
   ShowOptions();
}

// recursively search for repositories in a directory
void TPackageManagerMeta::RecursiveSearch(TString dir)
{
   TList *files = GetFiles(dir.Data());
   if (files) {
      TSystemFile *file;
      TString fname;
      TIter next(files);
      while ((file = (TSystemFile*)next())) {
         fname = file -> GetName();
         TString path;
         path.Form("%s/%s", dir.Data(), fname.Data());
         path.ReplaceAll("//", "/");
         if (fname != "." && fname != "..") {
            if (file -> IsDirectory()) {
               if (fname == PACK_DIR) {
                  TVirtualPackageManager *localRepo = new TPackageManagerDir(strdup(path.Data()));
                  std::cout << path.Data() << std::endl;
                  if (Add(localRepo) < 0)
                     delete localRepo;
               } else
                  RecursiveSearch(path.Data());
            }
         }
      }
   }
}

// search for repositories
void TPackageManagerMeta::Search(const char *dir)
{
   TString path = NormalizePath(dir);
   RecursiveSearch(path);
}
