#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_start_clicked();

    void on_loadConfig_clicked();

private:
    Ui::MainWindow *ui;
    QStringList states;
    QStringList alphabet;
    QMap<QString, QMap<QString, QString>> transitionFunction;
    QString startState;
    QStringList endStates;
    QStandardItemModel *model;

    bool parseJsonFile(const QString& filePath);
    void populateTable();
    void openFields();
};
#endif // MAINWINDOW_H
