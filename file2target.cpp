#include "file2target.h"

File2target::File2target(const QFileInfo &file,const QString &target)
{
    fileinfo=file;
    target_path=target;
}
