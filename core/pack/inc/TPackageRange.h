#ifndef ROOT_TPackageRange
#define ROOT_TPackageRange

#ifndef ROOT_TString
#include "TString.h"
#endif

#include "TSystem.h"
#include "TPackageVersion.h"
#include "TPackageUtils.h"
#include <iostream>
#include <stdlib.h>

#define GT "gt"
#define GTE "gte"
#define LT "lt"
#define LTE "lte"

class TPackageRange : public TObject {
    private:
        TString packName; // name of package
        TPackageVersion *lowVersion; // low end of version
        TPackageVersion *highVersion; // high end of version
        Bool_t leftOpen; // is the interval open on the low end
        Bool_t rightOpen; // is the interval open on the high end
    public:
        TPackageRange(TString name, TPackageVersion *infVers,
                            TPackageVersion *supVers, Bool_t openLeft, Bool_t openRight):
                        packName(name), lowVersion(infVers), highVersion(supVers),
                        leftOpen(openLeft), rightOpen(openRight) {}
        TPackageRange(const char *line);
        ~TPackageRange() { delete lowVersion; delete highVersion; }

        inline const char *GetPackName() const { return packName.Data(); }
        inline TPackageVersion *GetLowVersion() const { return lowVersion; }
        inline TPackageVersion *GetHighVersion() const { return highVersion; }
        inline Bool_t IsLeftOpen() const { return leftOpen; }
        inline Bool_t IsRightOpen() const { return rightOpen; }

        TString GetRange() const;
        Bool_t ContainsVersion(TPackageVersion *version) const;
        TPackageRange *GetIntersection(TPackageRange *range) const;

        static TPackageRange *CreateRange(const char *dep);

        ClassDefNV(TPackageRange, 1);
};

#endif
