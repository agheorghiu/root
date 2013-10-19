#ifndef ROOT_TPackageFile
#define ROOT_TPackageFile

#ifndef ROOT_TString
#include "TString.h"
#endif

#ifndef ROOT_TMD5
#include "TMD5.h"
#endif

#include "TSystem.h"

class TPackageFile : public TObject {
   private:
      TString fileName; // name of file
      TString filePath; // absolute file path
      TString relativePath; // relative path
      TMD5 *md5; // md5 checksum
      Long_t size; // file size in bytes
   public:
      TPackageFile(TString path);
      ~TPackageFile() { delete md5; };
      inline const char *GetFileName() const { return fileName.Data(); }
      inline Long_t GetFileSize() const { return size; }
      inline void SetFilePath(TString newPath) { filePath = newPath; }

      void MakeRelativePath(const char *baseDir);
      const char *GetFilePath() const;
      const char *GetMD5();

      // path to file uniquely identifies a file
      const char *GetName() const { return filePath.Data(); }

      ClassDefNV(TPackageFile, 1);
};

#endif
