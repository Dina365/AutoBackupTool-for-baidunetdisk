#ifndef UNBACKUPFILELIST_H
#define UNBACKUPFILELIST_H
#include <QList>
#include "file2target.h"


class UnbackupFileList
{
public:
    UnbackupFileList();
    void addFile2target(const QFileInfo &file,const QString &target);
    QList<File2target>::iterator deleteFile2target(QList<File2target>::iterator it);
    void deleteFirst();
    void deleteLast();
    QList<File2target>::iterator item(int n);
    QList<File2target> filelist;
    QList<File2target>::iterator end();
    QList<File2target>::iterator begin();
    int size();
    void clear();
};

#endif // UNBACKUPFILELIST_H
