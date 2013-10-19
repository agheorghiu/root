#include "TPackageRange.h"

// Create interval from dependency
TPackageRange *TPackageRange::CreateRange(const char *dep)
{
   TString strDep = dep;
   TString name, vers1, vers2;
   Bool_t leftOpen, rightOpen;
   Int_t leftSeparator, midSeparator, rightSeparator;
   Int_t majVers1, minVers1, majVers2, minVers2;
   if (strDep.Contains("[")) {
      leftSeparator = strDep.Index("[");
      leftOpen = kFALSE;
   }
   else if (strDep.Contains("(")) {
      leftSeparator = strDep.Index("(");
      leftOpen = kTRUE;
   }
   else {
      std::cout << "Invalid string. Missing left interval '[' or '('" << std::endl;
      return NULL;
   }
   if (strDep.Contains("-"))
      midSeparator = strDep.Index("-");
   else {
      std::cout << "Invalid string. Missing version separator '-'" << std::endl;
      return NULL;
   }
   if (strDep.Contains("]")) {
      rightSeparator = strDep.Index("]");
      rightOpen = kFALSE;
   }
   else if (strDep.Contains(")")) {
      rightSeparator = strDep.Index(")");
      rightOpen = kTRUE;
   }
   else {
      std::cout << "Invalid string. Missing right interval ']' or ')'" << std::endl;
      return NULL;
   }
   name = strDep(0, leftSeparator);
   vers1 = strDep(leftSeparator + 1, midSeparator);
   vers2 = strDep(midSeparator + 1, rightSeparator);
   if (sscanf(vers1.Data(), "%d.%d", &majVers1, &minVers1) != 2) {
      std::cout << "First version in interval specified incorrectly" << std::endl;
      return NULL;
   }
   if (sscanf(vers2.Data(), "%d.%d", &majVers2, &minVers2) != 2) {
      std::cout << "Second version in interval specified incorrectly" << std::endl;
      return NULL;
   }
   TPackageVersion *infVers = new TPackageVersion(name, majVers1, minVers1, TString(""));
   TPackageVersion *supVers = new TPackageVersion(name, majVers2, minVers2, TString(""));
   TPackageRange *interval = new TPackageRange(name, infVers, supVers, leftOpen, rightOpen);
   return interval;
}

// constructor
TPackageRange::TPackageRange(const char *line)
{
   std::string strLine(line);
   std::vector<std::string> tokens = split(strLine, ' ');
   if (tokens.size() != 8) {
      SetBit(kInvalidObject);
      return;
   }
   packName = tokens[0].c_str();
   if (TString(tokens[1].c_str()) == GT)
      leftOpen = kTRUE;
   else
      leftOpen = kFALSE;
   lowVersion = new TPackageVersion(packName, atoi(tokens[2].c_str()), atoi(tokens[3].c_str()), TString(""));
   highVersion = new TPackageVersion(packName, atoi(tokens[5].c_str()), atoi(tokens[6].c_str()), TString(""));
   if (TString(tokens[7].c_str()) == LT)
      rightOpen = kTRUE;
   else
      rightOpen = kFALSE;
}

// get string representation of interval
TString TPackageRange::GetRange() const
{
   TString line;
   line.Form("%s %s %d %d - %d %d %s", packName.Data(), (leftOpen ? GT : GTE),
         lowVersion -> GetMajorVersion(), lowVersion -> GetMinorVersion(),
         highVersion -> GetMajorVersion(), highVersion -> GetMinorVersion(),
         (rightOpen ? LT : LTE));
   return line;
}

// check if range contains version
Bool_t TPackageRange::ContainsVersion(TPackageVersion *version) const
{
   if (leftOpen && rightOpen) {
      if (version -> CompareTo(lowVersion) == 1 && version -> CompareTo(highVersion) == -1)
         return kTRUE;
      else
         return kFALSE;
   }
   if (leftOpen && !rightOpen) {
      if (version -> CompareTo(lowVersion) == 1 && (version -> CompareTo(highVersion) == -1
               || version -> CompareTo(highVersion) == 0))
         return kTRUE;
      else
         return kFALSE;
   }
   if (!leftOpen && rightOpen) {
      if ((version -> CompareTo(lowVersion) == 1 || version -> CompareTo(lowVersion) == 0)
            && version -> CompareTo(highVersion) == -1)
         return kTRUE;
      else
         return kFALSE;
   }
   if (!leftOpen && !rightOpen) {
      if ((version -> CompareTo(lowVersion) == 1 || version -> CompareTo(lowVersion) == 0)
            && (version -> CompareTo(highVersion) == -1 || version -> CompareTo(highVersion) == 0))
         return kTRUE;
      else
         return kFALSE;
   }
   return kFALSE;
}

// obtain intersection of this range with another range
TPackageRange *TPackageRange::GetIntersection(TPackageRange *range) const
{
   TPackageVersion *intersectLow, *intersectHigh;
   Bool_t openL, openR;
   intersectLow = TPackageVersion::Max(lowVersion, range -> GetLowVersion());
   intersectHigh = TPackageVersion::Min(highVersion, range -> GetHighVersion());
   if (intersectLow == NULL || intersectHigh == NULL || intersectLow -> CompareTo(intersectHigh) > 0)
      return NULL;
   if (lowVersion -> CompareTo(range -> GetLowVersion()) == 0)
      openL = leftOpen && range -> IsLeftOpen();
   else {
      if (intersectLow == lowVersion)
         openL = leftOpen;
      else
         openL = range -> IsLeftOpen();
   }
   if (highVersion -> CompareTo(range -> GetHighVersion()) == 0)
      openR = rightOpen && IsRightOpen();
   else {
      if (intersectHigh == highVersion)
         openR = rightOpen;
      else
         openR = range -> IsRightOpen();
   }
   TPackageRange *intersection = new TPackageRange(packName, intersectLow, intersectHigh, openL, openR);
   return intersection;
}
