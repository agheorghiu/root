#ifndef ROOT_TPackageManagerDir
#define ROOT_TPackageManagerDir

#include "TVirtualPackageManager.h"

class TPackageManagerDir : public TVirtualPackageManager {
   private:
      TString repoPath; // path to repository
      mutable TList *availablePacks; // list of available packages

      Bool_t Init(const char *dirPath);
      void BuildList() const;
      void ShowContents() const;
      void ShowPackages() const;
   public:
      TPackageManagerDir(const char *dirPath);
      Int_t Enable(const char *packName);
      Int_t Disable(const char *packName);
      Int_t Remove(const char *packName);
      Int_t Put() { return 0; }
      Int_t Get() { return 0; }
      TList *List();
      Bool_t ResolveDeps(TPackage *p);

      void Print(Option_t *option = "") const;
      const char *GetName() const { return repoPath.Data(); }

      ClassDef(TPackageManagerDir, 1);
};

#endif
