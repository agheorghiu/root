#include "TPackageFile.h"

TPackageFile::TPackageFile(TString path)
{
   if (gSystem -> IsAbsoluteFileName(path))
      filePath = path;
   else
      filePath.Form("%s/%s", gSystem -> WorkingDirectory(), path.Data());
   fileName = gSystem -> BaseName(path.Data());
   relativePath = TString((const char*)NULL);
   md5 = NULL;
   Long_t id, flags, modtime;
   gSystem -> GetPathInfo(path.Data(), &id, &size, &flags, &modtime);
}

const char *TPackageFile::GetMD5()
{
   if (!md5)
      md5 = TMD5::FileChecksum(filePath.Data());
   return md5 -> AsString();
}

const char *TPackageFile::GetFilePath() const
{
   if (relativePath.Length() > 0)
      return relativePath.Data();
   return filePath.Data();
}

void TPackageFile::MakeRelativePath(const char *baseDir)
{
   TString currentPath = filePath;
   TString strBaseDir = baseDir;
   if (currentPath.Index(baseDir) == 0) {
      relativePath = currentPath(strBaseDir.Length() + 1, currentPath.Length() - 1);
   }
}
