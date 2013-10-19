#ifndef ROOT_TProofPackageManagerProof
#define ROOT_TProofPackageManagerProof

#include "TVirtualPackageManager.h"

class TPackageManagerProof : public TVirtualPackageManager {

   public:
      TPackageManagerProof() {}
      Int_t Enable(const char *) { return 0; }
      Int_t Disable(const char *) { return 0; }
      Int_t Remove(const char *) { return 0; }
      Int_t Put() { return 0; }
      Int_t Get() { return 0; }
      TList *List() { return NULL; }
      Bool_t ResolveDeps(TPackage *) { return kTRUE; }

      ClassDef(TPackageManagerProof, 1);
};

#endif
