#ifndef FILESTRUCTURE
#define FILESTRUCTURE

#include <FileSystemObject.h>
#include <list>

struct FileStructure
{
    FileStructure() : firstFile(nullptr){}
    std::list<FileSystemObject> objects;
    FileSystemObject *firstFile;
};

#endif // FILESTRUCTURE

