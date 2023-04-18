#ifndef FILE2TARGET_H
#define FILE2TARGET_H
#include <QString>
#include <QFileInfo>

class File2target
{
public:
    File2target(const QFileInfo &file,const QString &target);
    QFileInfo fileinfo;//源文件
    QString target_path;//目的地目录
};

#endif // FILE2TARGET_H
