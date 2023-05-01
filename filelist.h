#ifndef FILELIST_H
#define FILELIST_H
#include <QString>
#include <QList>
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
        QList<file_datetime>::iterator deletefile_datetime(QList<file_datetime>::iterator it);
        void deleteFirst();
        void deleteLast();
        QList<file_datetime>::iterator item(int n);
        QList<file_datetime> filelist;
        QList<file_datetime>::iterator end();
        QList<file_datetime>::iterator begin();
        int size();
        void clear();

};

#endif // FILELIST_H
