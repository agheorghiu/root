#include "TPackageVersion.h"

// constructor
TPackageVersion::TPackageVersion(const char *fullName)
{
    TString strFullName = fullName;
    Int_t majVers = -1, minVers = -1;
    TString leftPart, rightPart;
    if (strFullName.Contains("-") == kFALSE) {
        std::cout << "Invalid name" << std::endl;
        SetBit(kInvalidObject);
        return;
    }
    leftPart = strFullName(0, strFullName.Index("-"));
    rightPart = strFullName(strFullName.Index("-") + 1, strFullName.Length() - 1);
    char patch[strFullName.Length()];
    memset(patch, 0, strFullName.Length());
    if (sscanf(rightPart.Data(), "%d.%d.%s", &majVers, &minVers, patch) < 3)
        if (sscanf(rightPart.Data(), "%d.%d", &majVers, &minVers) < 2) {
            std::cout << "Invalid name" << std::endl;
            SetBit(kInvalidObject);
            return;
        }
    packName = leftPart;
    majorVersion = majVers;
    minorVersion = minVers;
    patchVersion = patch;
}

// returns:
// -2 versoin objects have different names
// -1 if `this` is less than version
// 0 if `this` is equal to version
// 1 if `this` is greater than version
Int_t TPackageVersion::CompareTo(TPackageVersion *version) const
{
    if (packName != version -> GetPackName())
        return -2;

    if (majorVersion < version -> GetMajorVersion())
        return -1;
    if (majorVersion > version -> GetMajorVersion())
        return 1;

    if (minorVersion < version -> GetMinorVersion())
        return -1;
    if (minorVersion > version -> GetMinorVersion())
        return 1;

    return 0;
}

// get the smallest of two versions
TPackageVersion *TPackageVersion::Min(TPackageVersion *ver1, TPackageVersion *ver2)
{
    TString name = ver1 -> GetPackName();
    if (name != ver2 -> GetPackName())
        return NULL;
    if (ver1 -> CompareTo(ver2) <= 0)
        return ver1;
    else
        return ver2;
}

// get the largest of two versions
TPackageVersion *TPackageVersion::Max(TPackageVersion *ver1, TPackageVersion *ver2)
{
    TString name = ver1 -> GetPackName();
    if (name != ver2 -> GetPackName())
        return NULL;
    if (ver1 -> CompareTo(ver2) >= 0)
        return ver1;
    else
        return ver2;
}
