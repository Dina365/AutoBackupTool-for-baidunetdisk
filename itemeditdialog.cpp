#include "itemeditdialog.h"
#include "ui_itemeditdialog.h"

ItemEditDialog::ItemEditDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ItemEditDialog)
{
    ui->setupUi(this);
    setFixedSize(width(),height());
    ui->buttonBox->addButton("确认",QDialogButtonBox::YesRole);
    ui->buttonBox->addButton("取消",QDialogButtonBox::NoRole);
}

ItemEditDialog::ItemEditDialog(QWidget *parent ,const QTableWidgetItem* _item,QString* _ret):
    QDialog(parent),
    ui(new Ui::ItemEditDialog),
    item(_item),
    ret(_ret)
{
    ui->setupUi(this);
    ui->textEdit->setText(_item->text());
    ui->buttonBox->addButton("取消",QDialogButtonBox::RejectRole);
    ui->buttonBox->addButton("确认",QDialogButtonBox::AcceptRole);
}

ItemEditDialog::~ItemEditDialog()
{
    delete ui;
}

void ItemEditDialog::on_textEdit_textChanged()
{

}

void ItemEditDialog::on_pushButton_clicked()
{
    QString ret = QFileDialog::getExistingDirectory(this);
    if(ret.isEmpty())return;
    ui->textEdit->setText(ret);
}

void ItemEditDialog::on_buttonBox_accepted()
{
    QFileInfo path(ui->textEdit->toPlainText());
    if(!path.exists()){
        QMessageBox::critical(this,"错误","该路径不存在，请重新输入。","确认");
        ui->textEdit->setText(item->text());
        return;
    }else if(!path.isDir()){
        QMessageBox::critical(this,"错误","请输入一个文件夹的路径。","确认");
        ui->textEdit->setText(item->text());
        return;
    }
    *ret=ui->textEdit->toPlainText();
}
