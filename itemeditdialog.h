#ifndef ITEMEDITDIALOG_H
#define ITEMEDITDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>

namespace Ui {
class ItemEditDialog;
}

class ItemEditDialog : public QDialog
{
    Q_OBJECT
private:
    Ui::ItemEditDialog *ui;
public:
    explicit ItemEditDialog(QWidget *parent = 0);
    ItemEditDialog(QWidget *parent,const QTableWidgetItem* item,QString* ret);
    ~ItemEditDialog();
    const QTableWidgetItem* item;
    QString* ret;
private slots:
    void on_textEdit_textChanged();


    void on_pushButton_clicked();
    void on_buttonBox_accepted();
};

#endif // ITEMEDITDIALOG_H
