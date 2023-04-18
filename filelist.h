#ifndef FILELIST_H
#define FILELIST_H
#include <QString>
#include <QLinkedList>
class file_datetime{
public:
    QString file;
    QString date;
    QString time;
    file_datetime(const QString& file,const QString& date,const QString& time){
        this->file=file;
        this->date=date;
        this->time=time;
    }


};

class FileList
{

    public:
        FileList();
        void addFile_datetime(const QString &file,const QString &date,const QString &time);
        QLinkedList<file_datetime>::iterator deletefile_datetime(QLinkedList<file_datetime>::iterator it);
        void deleteFirst();
        void deleteLast();
        QLinkedList<file_datetime>::iterator item(int n);
        QLinkedList<file_datetime> filelist;
        QLinkedList<file_datetime>::iterator end();
        QLinkedList<file_datetime>::iterator begin();
        int size();
        void clear();

};

#endif // FILELIST_H
