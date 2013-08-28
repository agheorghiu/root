#ifndef ROOT_TPackageVersion
#define ROOT_TPackageVersion

#ifndef ROOT_TString
#include "TString.h"
#endif

#include "TSystem.h"
#include <iostream>

class TPackageVersion : public TObject {
    private:
        TString packName; // name of package
        Int_t majorVersion; // major version number
        Int_t minorVersion; // minor version number
        TString patchVersion; // patch version string
    public:
        TPackageVersion(TString name, Int_t majVers, Int_t minVers, TString patch):
            packName(name), majorVersion(majVers), minorVersion(minVers), patchVersion(patch) {}
        TPackageVersion(const char *fullName);
        ~TPackageVersion() {};

        inline const char *GetPackName() const { return packName.Data(); }
        inline Int_t GetMajorVersion() const { return majorVersion; }
        inline Int_t GetMinorVersion() const { return minorVersion; }
        inline const char *GetPatchVersion() const { return patchVersion.Data(); }

        Int_t CompareTo(TPackageVersion *version) const;
        static TPackageVersion *Min(TPackageVersion *ver1, TPackageVersion *ver2);
        static TPackageVersion *Max(TPackageVersion *ver1, TPackageVersion *ver2);

        ClassDefNV(TPackageVersion, 1);
};

#endif
