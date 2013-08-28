#ifndef ROOT_TVirtualPackageManager
#define ROOT_TVirtualPackageManager

#include "TPackage.h"

class TVirtualPackageManager : public TObject {
    public:
        virtual Int_t Enable(const char *packName) = 0;
        virtual Int_t Disable(const char *packName) = 0;
        virtual Int_t Remove(const char *packName) = 0;
        virtual Int_t Put() = 0;
        virtual Int_t Get() = 0;
        virtual TList *List() = 0;
        virtual Bool_t ResolveDeps(TPackage *package) = 0;
    ClassDef(TVirtualPackageManager, 1);
};

#endif
