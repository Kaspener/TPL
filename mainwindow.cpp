#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->log->hide();
    ui->start->hide();
    ui->slider->hide();
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::parseJsonFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Не удалось открыть файл:" << filePath;
        return false;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(fileData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Ошибка парсинга JSON:" << parseError.errorString();
        return false;
    }

    if (!jsonDoc.isObject()) {
        qWarning() << "JSON не является объектом!";
        return false;
    }

    QJsonObject jsonObj = jsonDoc.object();

    if (jsonObj.contains("states") && jsonObj["states"].isArray()) {
        QJsonArray statesArray = jsonObj["states"].toArray();
        states.clear();
        for (const QJsonValue &value : statesArray) {
            if (value.isString()) {
                states.append(value.toString());
            }
        }
        qDebug() << "States:" << states;
    }
    else{
        qDebug() << "Массив 'states' не найден или не является массивом!";
        return false;
    }

    if (jsonObj.contains("alphabet") && jsonObj["alphabet"].isArray()) {
        QJsonArray alphabetArray = jsonObj["alphabet"].toArray();
        alphabet.clear();
        for (const QJsonValue &value : alphabetArray) {
            if (value.isString()) {
                alphabet.append(value.toString());
            }
        }
        if (!alphabet.contains("λ")){
            alphabet.push_back("λ");
        }
        qDebug() << "Alphabet:" << alphabet;
    }
    else{
        qDebug() << "Массив 'alphabet' не найден или не является массивом!";
        return false;
    }

    if (jsonObj.contains("in_stack") && jsonObj["in_stack"].isArray()) {
        QJsonArray inStackArray = jsonObj["in_stack"].toArray();
        in_stack.clear();
        for (const QJsonValue &value : inStackArray) {
            if (value.isString()) {
                in_stack.append(value.toString());
            }
        }
        qDebug() << "In stack:" << in_stack;
    }
    else{
        qDebug() << "Массив 'in_stack' не найден или не является массивом!";
        return false;
    }

    if (jsonObj.contains("rules") && jsonObj["rules"].isArray()) {
        QJsonArray rulesArray = jsonObj["rules"].toArray();
        transitionFunction.clear();

        for (const QJsonValue &value : rulesArray) {
            if (!value.isArray()) {
                qDebug() << "Элемент не является массивом!";
                return false;
            }

            QJsonArray rule = value.toArray();
            if (rule.size() != 5) {
                qDebug() << "Неверный размер массива правила!";
                return false;
            }
            QString key1 = rule[0].toString();
            QString key2 = rule[1].toString();
            QString key3 = rule[2].toString();
            QString value1 = rule[3].toString();
            QString value2 = rule[4].toString();

            transitionFunction[std::make_tuple(key1, key2, key3)] = std::make_tuple(value1, value2);
        }
    }
    else{
        qDebug() << "Массив 'rules' не найден или не является массивом!";
        return false;
    }

    if (jsonObj.contains("start") && jsonObj["start"].isString()) {
        startState = jsonObj["start"].toString();
        qDebug() << "Start state:" << startState;
    }
    else{
        qDebug() << "значение 'start' не найден или не является массивом!";
        return false;
    }

    if (jsonObj.contains("start_stack") && jsonObj["start_stack"].isString()) {
        startStack = jsonObj["start_stack"].toString();
        qDebug() << "Start stack:" << startStack;
    }
    else{
        qDebug() << "значение 'start_stack' не найден или не является массивом!";
        return false;
    }

    if (jsonObj.contains("ends") && jsonObj["ends"].isArray()) {
        QJsonArray endsArray = jsonObj["ends"].toArray();
        endStates.clear();
        for (const QJsonValue &value : endsArray) {
            if (value.isString()) {
                endStates.append(value.toString());
            }
        }
        qDebug() << "End states:" << endStates;
    }
    else{
        qDebug() << "Массив 'rules' не найден или не является массивом!";
        return false;
    }
    return true;
}

void MainWindow::populateList()
{
    if (ui->list->model()) {
        delete ui->list->model();
    }
    model = new QStandardItemModel(this);

    QStringList list;

    for (auto it = transitionFunction.begin(); it != transitionFunction.end(); ++it) {
        auto key = it.key();
        auto value = it.value();

        QString keyStr = QString("(%1, %2, %3)")
                             .arg(std::get<0>(key))
                             .arg(std::get<1>(key))
                             .arg(std::get<2>(key));

        QString valueStr = QString("(%1, %2)")
                               .arg(std::get<0>(value))
                               .arg(std::get<1>(value));

        QStandardItem *item = new QStandardItem(QString("%1 -> %2").arg(keyStr, valueStr));

        model->appendRow(item);
    }

    ui->list->setModel(model);
}

void MainWindow::openFields()
{
    ui->start->setEnabled(true);
    ui->loadConfig->setEnabled(true);
    ui->command->setEnabled(true);
}

QString printStack(const QStack<QString>& stack){
    QString result{};
    for(auto it = stack.rbegin(); it != stack.rend(); ++it){
        result += *it;
    }
    return result;
}

void MainWindow::on_start_clicked()
{
    ui->start->setEnabled(false);
    ui->loadConfig->setEnabled(false);
    ui->command->setEnabled(false);
    ui->log->clear();
    QString command = ui->command->text();
    QString state = startState;
    QStack<QString> stack;
    stack.push(startStack);
    QString newState;
    QString stackOperation;
    bool reload = false;
    while(stack.size() > 0){
        if(command.isEmpty()) command = "λ";
        QString string = "<font color='green'>(" + state + ", " + command + ", " + printStack(stack) + ")</font>";
        ui->log->append(string);
        if (!states.contains(state)){
            QString error = "<font color='red'>δ(" + state + "," + command[0] + ", " + stack.top() + ") -> Состояния {" + state + "} не существует!!</font>";
            ui->log->append(error);
            openFields();
            return;
        }
        if (!alphabet.contains(QString(command[0]))){
            QString error = "<font color='red'>δ(" + state + "," + command[0] + ", " + stack.top() + ") -> Символ {" + command[0] + "} не входит в алфавит!</font>";
            ui->log->append(error);
            openFields();
            return;
        }
        if (!in_stack.contains(stack.top())){
            QString error = "<font color='red'>δ(" + state + "," + command[0] + ", " + stack.top() + ") -> Символ {" + stack.top() + "} не входит в алфавит стека!</font>";
            ui->log->append(error);
            openFields();
            return;
        }
        QString searchString = QString("(%1, %2, %3)").arg(state, command[0], stack.top());

        int row = 0;
        QBrush color = model->item(0)->background();
        QStandardItem *item = model->item(0);

        for (row = 0; row < model->rowCount(); ++row) {
            item = model->item(row);
            QString currentText = item->text();

            if (currentText.startsWith(searchString)) {
                item->setBackground(QBrush(QColor(144, 238, 144)));
                break;
            }
        }
        if (!transitionFunction.contains(std::make_tuple(state, command[0], stack.top()))){
            QString error = "<font color='red'>Не существует правила перехода (" + state + "," + command[0] + ", " + stack.top() + "). Цепочка не принадлежит заданному ДМПА!</font>";
            ui->log->append(error);
            openFields();
            return;
        }
        QString left = "<font color='green'>δ(" + state + "," + command[0] + "," + stack.top() + ") -> ";
        std::tie(newState, stackOperation) = transitionFunction[std::make_tuple(state, command[0], stack.top())];

        if (stackOperation.length() > 1){
            if (QString(stackOperation.back()) == stack.top()){
                stackOperation.chop(1);
            }
            for(auto it = stackOperation.rbegin(); it != stackOperation.rend(); it++){
                stack.push(QString(*it));
            }
        }
        else{
            if (stackOperation == "ε")
            {
                stack.pop();
            }
            else
            {
                stack.pop();
                stack.push(stackOperation);
            }
        }

        state = newState;

        command.removeFirst();
        QString right;
        if (command.isEmpty()) command = "λ";
        right = "Новое состояние {" + state + "} оставшаяся цепочка - " + command + "</font>";
        ui->log->append(left + right);
        ui->log->append("<font color='green'>Стек (" + printStack(stack) +")");
        QTime dieTime= QTime::currentTime().addMSecs(ui->slider->value());
        while (QTime::currentTime() < dieTime)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        item->setBackground(color);
    }
    if (command!="λ"){
        QString error = "<font color='red'>В цепочке остались символы: " + command + ". Цепочка не принадлежит заданному ДМПА!</font>";
        ui->log->append(error);
        openFields();
        return;
    }

    if (!endStates.contains(state)){
       QString error = "<font color='red'>Cостояние {" + state + "} не является конечным. Цепочка не принадлежит заданному ДМПА!</font>";
       ui->log->append(error);
        openFields();
        return;
    }
    ui->log->append("<font color='green'>Цепочка принадлежит заданному ДМПА!</font>");
    openFields();
}


void MainWindow::on_loadConfig_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Выбрать файл", "", "JSON Files (*.json)");
    if (!fileName.isEmpty()) {
        parseJsonFile(fileName);
        populateList();
        ui->log->show();
        ui->log->clear();
        ui->start->show();
        ui->slider->show();
    }
}

