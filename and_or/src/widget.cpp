#include "widget.h"
#include "ui_widget.h"
#include <QSettings>
#include <QFileDialog>
#include <QMenu>
#include <QByteArray>
#include <QMessageBox>
#include <omp.h>

enum Operations{ AND, OR };
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    // Разрешаем собственное контекстное меню
    ui->inputPTE->setContextMenuPolicy( Qt::CustomContextMenu );
    setupInputPTEContextMenu();

    QSettings s;
    outputFileName =     s.value( "outputFileName" ).toString();
    ui->outputLED->setText( outputFileName );

    this->restoreGeometry( s.value( "geometry" ).toByteArray() );

    inputFileNames = s.value( "inputFileNames" ).toStringList();
    ui->inputPTE->setPlainText( inputFileNames.join( '\n' ) );

    bool andChecked = s.value( "rb" ).toBool();
    ui->andRB->setChecked( andChecked );
    ui->orRB->setChecked( !andChecked );
}

Widget::~Widget()
{
    QSettings s;
    s.setValue( "geometry"      , this->saveGeometry() );
    s.setValue( "outputFileName", outputFileName );
    s.setValue( "inputFileNames", inputFileNames );
    s.setValue( "rb"            , ui->andRB->isChecked() );
    delete ui;
}

void Widget::on_outputPBN_clicked()
{
    QString f = QFileDialog::getSaveFileName(
        this,                                      // Родительский виджет
        "Сохранить файл",                          // Заголовок окна
        QDir::homePath(),                          // Начальная папка
        "Текстовые файлы (*.txt);;Все файлы (*.*)" // Фильтр
        );
    if( !f.isEmpty() ){
        outputFileName = f;
        ui->outputLED->setText( f );
    }
}


void Widget::on_inputPBN_clicked()
{
    QStringList f = QFileDialog::getOpenFileNames(
        this,
        "Выберите файлы",
        QDir::homePath(),
        "Все файлы (*.*)"
        );
    if( !f.isEmpty() ){
        inputFileNames = f;
        ui->inputPTE->setPlainText( f.join( '\n' ) );
    }
}
void Widget::setupInputPTEContextMenu()
{
    // Разрешаем «кастомное» меню, чтобы Qt не показывал своё
    ui->inputPTE->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->inputPTE, &QPlainTextEdit::customContextMenuRequested,
            this, [this](const QPoint &pos) {
                // Находим номер строки под курсором
                QTextCursor cursor = ui->inputPTE->cursorForPosition(pos);
                int lineNumber = cursor.blockNumber();

                // Если номер валиден — удаляем из списка и обновляем PTE
                if ( lineNumber >= 0 && lineNumber < inputFileNames.size() ) {
                    inputFileNames.removeAt( lineNumber );
                    ui->inputPTE->setPlainText( inputFileNames.join( '\n' ) );
                }
            });
}

void Widget::on_executePBN_clicked()
{

    if ( outputFileName.isEmpty() ){
        QMessageBox::critical( this, "Ошибка", "Не выбран выходной файл" );
        return;
    }
    QFile outputFile( outputFileName );

    int totalFiles = inputFileNames.size();
    if ( totalFiles == 0 ){
        QMessageBox::critical( this, "Ошибка", "Не выбраны исходные файлы" );
        return;
    }
    if ( totalFiles == 1 ){
        QMessageBox::critical( this, "Ошибка", "Недостаточно исходных файлов" );
        return;
    }

    ui->executePBR->setValue(0);
    if ( !outputFile.open( QIODevice::WriteOnly | QIODevice::Truncate ) ) {
        QMessageBox::critical(this, "Ошибка","Не удается открыть выходной файл");
        return;
    }
    QList<QFile*> inputFiles;
    bool fIter = true;
    qint64 minSize;
    QStringList notOpen;
    for ( const QString &path : std::as_const( inputFileNames ) ) {
        QFile *f = new QFile(path);
        if( fIter ){
            fIter = false;
            minSize = f->size();
        }
        else{
            minSize = minSize < f->size() ? minSize : f->size();
        }
        if ( f->open( QIODevice::ReadOnly ) ) {
            inputFiles.append(f);
        } else {
            notOpen.append( path );
            delete f;
        }
    }
    if( notOpen.size() != 0 ){
        if(notOpen.size() == 1 )
            QMessageBox::critical( this, "Ошибка", "Не удается открыть файл:\n" + notOpen[0] );
        else
            QMessageBox::critical( this, "Ошибка", "Не удается открыть файлы:\n" + notOpen.join(", \n") );
        return;
    }
    bool op = ui->andRB->isChecked() ? AND : OR;
    QByteArray inputBuf1, inputBuf2;
    qint64 totalRead = 0;
    while( totalRead < minSize ){
        inputBuf1 = inputFiles[0]->read( BUF_SIZE < minSize - totalRead ? BUF_SIZE : minSize - totalRead );
        for ( int i = 1; i < totalFiles; ++i ) {
            inputBuf2 = inputFiles[i]->read( inputBuf1.size() );
            if( op == AND ){
                #pragma omp parallel for
                for ( int i = 0; i < inputBuf1.size(); ++i )
                    inputBuf1[i] = inputBuf1[i] & inputBuf2[i];
            }
            else{
                #pragma omp parallel for
                for ( int i = 0; i < inputBuf1.size(); ++i )
                    inputBuf1[i] = inputBuf1[i] | inputBuf2[i];
            }
        }
        totalRead += inputBuf1.size();
        outputFile.write( inputBuf1 );
        ui->executePBR->setValue( 100*totalRead/minSize );
        qApp->processEvents();
    }
    ui->executePBR->setValue( 100 );
    outputFile.close();
    for (QFile *f : std::as_const(inputFiles))   {
        f->close();
        delete f;
    }
}

