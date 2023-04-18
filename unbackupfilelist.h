#ifndef UNBACKUPFILELIST_H
#define UNBACKUPFILELIST_H
#include <QLinkedList>
#include "file2target.h"


class UnbackupFileList
{
public:
    UnbackupFileList();
    void addFile2target(const QFileInfo &file,const QString &target);
    QLinkedList<File2target>::iterator deleteFile2target(QLinkedList<File2target>::iterator it);
    void deleteFirst();
    void deleteLast();
    QLinkedList<File2target>::iterator item(int n);
    QLinkedList<File2target> filelist;
    QLinkedList<File2target>::iterator end();
    QLinkedList<File2target>::iterator begin();
    int size();
    void clear();
};

#endif // UNBACKUPFILELIST_H
