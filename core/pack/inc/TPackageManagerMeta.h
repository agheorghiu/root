#ifndef ROOT_TPackageManagerMeta
#define ROOT_TPackageManagerMeta

#include "TVirtualPackageManager.h"
#include "TPackageManagerDir.h"
#include "TPackageManagerProof.h"
#include "TMap.h"

class TPackageManagerMeta : public TVirtualPackageManager {
   private:
      TList *enabledPacks;
      TList *packMans;
      TMap *reposMap;
      TVirtualPackageManager *GetRepo(TPackage *pack);
      TVirtualPackageManager *GetRepo(const char *dir);
      TPackage *LocatePackage(const char *packName);
      void ShowOptions() const;
      void ShowEnabledPackages() const;
      void ShowAvailablePackages() const;
      void ShowPackageManagers() const;

      void RecursiveSearch(TString dir);
   public:
      TPackageManagerMeta();
      Int_t Add(TPackage *pack);
      Int_t Add(TVirtualPackageManager *packMan);
      Int_t Enable(const char *packName);
      Int_t Disable(const char *packName);
      Int_t Remove(const char *packName);
      Int_t Put();
      Int_t Get();
      TList *List();


      Bool_t ResolveDeps(TPackage *package);
      void Search(const char *dir);
      void Print(Option_t *option = "") const;

      ClassDef(TPackageManagerMeta, 1);
};

R__EXTERN TPackageManagerMeta *gPackMan;

#endif
