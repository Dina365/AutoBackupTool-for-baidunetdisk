#include "filelist.h"

FileList::FileList()
{

}

void FileList::addFile_datetime(const QString &file,const QString &date,const QString &time){
    filelist.push_back(file_datetime(file,date,time));
}

QLinkedList<file_datetime>::iterator FileList::deletefile_datetime(QLinkedList<file_datetime>::iterator it){
    return filelist.erase(it);
}
void FileList::deleteFirst(){
    if(!filelist.isEmpty()) filelist.removeFirst();
}
void FileList::deleteLast(){
    if(!filelist.isEmpty()) filelist.removeLast();
}

QLinkedList<file_datetime>::iterator FileList::item(int n){
    if(n>=0&&n<filelist.size()){
        QLinkedList<file_datetime>::iterator pos=filelist.begin();
        for(int i=0;i<n;i++){
            pos++;
        }
        return pos;
    }else return filelist.end();
}
QLinkedList<file_datetime>::iterator FileList::begin(){
    return filelist.begin();
}

QLinkedList<file_datetime>::iterator FileList::end(){
    return filelist.end();
}
int FileList::size(){
    return filelist.size();
}

void FileList::clear(){
    filelist.clear();
}
