#include "mainwindow.h"
#include "account_system.h"
#include <QMessageBox>
#include <QDateTime>
#include <QHeaderView>
#include <QApplication>
#include <QFile>
#include <QUrl>
#include <QDebug>
#include <cstdlib>
#include <memory>
#include <thread>
#include <chrono>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>

std::string timeToStr(time_t t);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_isLoggedIn(false)
    , m_running(true)
    , m_isSoundPlaying(false)
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // ========== 登录区域 ==========
    QGroupBox *loginBox = new QGroupBox("账户管理");
    QHBoxLayout *loginLayout = new QHBoxLayout;

    loginLayout->addWidget(new QLabel("用户名："));
    m_usernameEdit = new QLineEdit;
    loginLayout->addWidget(m_usernameEdit);

    loginLayout->addWidget(new QLabel("密码："));
    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    loginLayout->addWidget(m_passwordEdit);

    m_loginButton = new QPushButton("登录");
    m_loginButton->setStyleSheet("background-color: #4CAF50; color: white;");
    loginLayout->addWidget(m_loginButton);
    connect(m_loginButton, &QPushButton::clicked, this, &MainWindow::onLoginClicked);

    m_registerButton = new QPushButton("注册");
    loginLayout->addWidget(m_registerButton);
    connect(m_registerButton, &QPushButton::clicked, this, &MainWindow::onRegisterClicked);

    m_logoutButton = new QPushButton("退出登录");
    m_logoutButton->setStyleSheet("background-color: #f44336; color: white;");
    loginLayout->addWidget(m_logoutButton);
    connect(m_logoutButton, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);

    m_loginStatusLabel = new QLabel("未登录");
    m_loginStatusLabel->setStyleSheet("color: red;");
    loginLayout->addWidget(m_loginStatusLabel);

    loginBox->setLayout(loginLayout);
    mainLayout->addWidget(loginBox);

    // ========== 任务输入区域（语音按钮在任务名称后面） ==========
    QGroupBox *taskBox = new QGroupBox("添加任务");
    QHBoxLayout *taskLayout = new QHBoxLayout;

    taskLayout->addWidget(new QLabel("任务名称："));
    m_taskNameEdit = new QLineEdit;
    m_taskNameEdit->setMinimumWidth(120);
    taskLayout->addWidget(m_taskNameEdit);

    // ★★★ 语音按钮 ★★★
    m_voiceButton = new QPushButton("🎤");
    m_voiceButton->setFixedSize(32, 28);
    m_voiceButton->setStyleSheet("background-color: #9C27B0; color: white; border-radius: 5px; font-size: 14px;");
    m_voiceButton->setToolTip("语音输入任务名称");
    taskLayout->addWidget(m_voiceButton);
    connect(m_voiceButton, &QPushButton::clicked, this, &MainWindow::onVoiceInputClicked);

    taskLayout->addWidget(new QLabel("日期："));
    m_dateEdit = new QDateEdit;
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDate(QDate::currentDate());
    taskLayout->addWidget(m_dateEdit);
    connect(m_dateEdit, &QDateEdit::dateChanged, this, &MainWindow::onDateChanged);

    taskLayout->addWidget(new QLabel("时间："));
    m_timeEdit = new QTimeEdit;
    m_timeEdit->setTime(QTime::currentTime());
    taskLayout->addWidget(m_timeEdit);

    taskLayout->addWidget(new QLabel("优先级："));
    m_priorityComboBox = new QComboBox;
    m_priorityComboBox->addItems({"高", "中", "低"});
    taskLayout->addWidget(m_priorityComboBox);

    taskLayout->addWidget(new QLabel("分类："));
    m_categoryComboBox = new QComboBox;
    m_categoryComboBox->addItems({"学习", "娱乐", "生活"});
    taskLayout->addWidget(m_categoryComboBox);

    taskLayout->addWidget(new QLabel("提前提醒（分钟）："));
    m_remindMinutesSpinBox = new QSpinBox;
    m_remindMinutesSpinBox->setRange(0, 60);
    m_remindMinutesSpinBox->setValue(5);
    taskLayout->addWidget(m_remindMinutesSpinBox);

    m_addTaskButton = new QPushButton("添加任务");
    m_addTaskButton->setStyleSheet("background-color: #2196F3; color: white;");
    taskLayout->addWidget(m_addTaskButton);
    connect(m_addTaskButton, &QPushButton::clicked, this, &MainWindow::onAddTaskClicked);

    taskBox->setLayout(taskLayout);
    mainLayout->addWidget(taskBox);

    // ========== 按月查看 ==========
    QGroupBox *monthBox = new QGroupBox("按月查看");
    QHBoxLayout *monthLayout = new QHBoxLayout;

    monthLayout->addWidget(new QLabel("年份："));
    m_yearComboBox = new QComboBox;
    int currentYear = QDate::currentDate().year();
    for (int y = currentYear - 5; y <= currentYear + 5; ++y) {
        m_yearComboBox->addItem(QString::number(y));
    }
    m_yearComboBox->setCurrentText(QString::number(currentYear));
    monthLayout->addWidget(m_yearComboBox);

    monthLayout->addWidget(new QLabel("月份："));
    m_monthComboBox = new QComboBox;
    m_monthComboBox->addItems({
        "一月", "二月", "三月", "四月", "五月", "六月",
        "七月", "八月", "九月", "十月", "十一月", "十二月"
    });
    m_monthComboBox->setCurrentIndex(QDate::currentDate().month() - 1);
    monthLayout->addWidget(m_monthComboBox);

    m_showMonthButton = new QPushButton("查看本月任务");
    m_showMonthButton->setStyleSheet("background-color: #9C27B0; color: white;");
    monthLayout->addWidget(m_showMonthButton);
    connect(m_showMonthButton, &QPushButton::clicked, this, &MainWindow::onShowMonthClicked);

    m_taskCountLabel = new QLabel("任务数：0");
    m_taskCountLabel->setStyleSheet("color: #9C27B0; font-weight: bold;");
    monthLayout->addWidget(m_taskCountLabel);

    monthLayout->addStretch();
    monthBox->setLayout(monthLayout);
    mainLayout->addWidget(monthBox);

    // ========== 日历视图 ==========
    QGroupBox *calendarBox = new QGroupBox("日历");
    QVBoxLayout *calendarLayout = new QVBoxLayout;
    m_calendarWidget = new QCalendarWidget;
    m_calendarWidget->setGridVisible(true);
    calendarLayout->addWidget(m_calendarWidget);
    connect(m_calendarWidget, &QCalendarWidget::clicked, this, &MainWindow::onCalendarClicked);
    calendarBox->setLayout(calendarLayout);
    mainLayout->addWidget(calendarBox);

    // ========== 任务列表 ==========
    QGroupBox *listBox = new QGroupBox("任务列表");
    QVBoxLayout *listLayout = new QVBoxLayout;

    QHBoxLayout *btnLayout = new QHBoxLayout;
    m_refreshButton = new QPushButton("刷新");
    btnLayout->addWidget(m_refreshButton);
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);

    m_showAllButton = new QPushButton("显示所有");
    btnLayout->addWidget(m_showAllButton);
    connect(m_showAllButton, &QPushButton::clicked, this, &MainWindow::onShowAllClicked);

    m_deleteTaskButton = new QPushButton("删除选中");
    m_deleteTaskButton->setStyleSheet("background-color: #ff9800; color: white;");
    btnLayout->addWidget(m_deleteTaskButton);
    connect(m_deleteTaskButton, &QPushButton::clicked, this, &MainWindow::onDeleteTaskClicked);

    m_editTaskButton = new QPushButton("修改任务");
    m_editTaskButton->setStyleSheet("background-color: #4CAF50; color: white;");
    btnLayout->addWidget(m_editTaskButton);
    connect(m_editTaskButton, &QPushButton::clicked, this, &MainWindow::onEditTaskClicked);

    btnLayout->addStretch();
    listLayout->addLayout(btnLayout);

    m_taskTableWidget = new QTableWidget;
    m_taskTableWidget->setColumnCount(6);
    m_taskTableWidget->setHorizontalHeaderLabels({"ID", "任务名称", "开始时间", "优先级", "分类", "提醒时间"});
    m_taskTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_taskTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_taskTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    listLayout->addWidget(m_taskTableWidget);

    listBox->setLayout(listLayout);
    mainLayout->addWidget(listBox);

    // ========== 音频 ==========
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.7);

    QString soundFile1 = "../alert.wav";

    if (QFile::exists(soundFile1)) {
        m_soundFilePath = soundFile1;
    } else {
        m_soundFilePath = "";
    }

    if (!m_soundFilePath.isEmpty()) {
        m_mediaPlayer->setSource(QUrl::fromLocalFile(m_soundFilePath));
    }

    setWindowTitle("MySchedule - 智能日程管理");
    toggleControls(false);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::onReminderTimer);
    m_timer->start(100);
}

MainWindow::~MainWindow()
{
    m_running = false;
    if (m_reminderThread.joinable()) {
        m_reminderThread.join();
    }
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
        delete m_mediaPlayer;
    }
    if (m_audioOutput) {
        delete m_audioOutput;
    }
}

// ========== 按月查看功能 ==========

QString MainWindow::getMonthName(int month) const
{
    QStringList monthNames = {"一月", "二月", "三月", "四月", "五月", "六月",
                              "七月", "八月", "九月", "十月", "十一月", "十二月"};
    if (month >= 1 && month <= 12) {
        return monthNames[month - 1];
    }
    return "Unknown";
}

std::vector<Task> MainWindow::getTasksForMonth(int year, int month) const
{
    std::vector<Task> monthTasks;

    if (!m_taskManager) {
        return monthTasks;
    }

    auto allTasks = m_taskManager->getAllTasks();

    QDate startDate(year, month, 1);
    QDate endDate(year, month, startDate.daysInMonth());

    QDateTime startDateTime(startDate, QTime(0, 0, 0));
    QDateTime endDateTime(endDate, QTime(23, 59, 59));

    time_t startTime = startDateTime.toSecsSinceEpoch();
    time_t endTime = endDateTime.toSecsSinceEpoch();

    for (const auto& task : allTasks) {
        if (task.startTime >= startTime && task.startTime <= endTime) {
            monthTasks.push_back(task);
        }
    }

    std::sort(monthTasks.begin(), monthTasks.end(), [](const Task& a, const Task& b) {
        return a.startTime < b.startTime;
    });

    return monthTasks;
}

void MainWindow::onShowMonthClicked()
{
    if (!m_isLoggedIn || !m_taskManager) {
        showMessage("错误", "请先登录！", true);
        return;
    }

    int year = m_yearComboBox->currentText().toInt();
    int month = m_monthComboBox->currentIndex() + 1;

    auto monthTasks = getTasksForMonth(year, month);

    if (monthTasks.empty()) {
        showMessage("本月任务", QString("%1年%2 暂无任务").arg(year).arg(getMonthName(month)));
        return;
    }

    // 创建对话框
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(QString("%1年%2 任务列表").arg(year).arg(getMonthName(month)));

    dialog->setMinimumWidth(340);
    dialog->setMinimumHeight(100);
    dialog->setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(dialog);

    // 标题居中
    QLabel *titleLabel = new QLabel(QString("共 %1 个任务").arg(monthTasks.size()));
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 13px; font-weight: bold; padding: 5px;");
    layout->addWidget(titleLabel);

    // ★★★ 表格居中 ★★★
    QTableWidget *table = new QTableWidget(dialog);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels(QStringList() << "ID" << "任务名称" << "开始时间" << "优先级" << "分类");
    table->setRowCount(monthTasks.size());
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // 表格字体调小
    table->setFont(QFont("Arial", 9));

    for (size_t i = 0; i < monthTasks.size(); ++i) {
        const auto& task = monthTasks[i];
        table->setItem(i, 0, new QTableWidgetItem(QString::number(task.id)));
        table->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(task.name)));
        table->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(timeToStr(task.startTime))));
        table->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(Task::priorityToString(task.priority))));
        table->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(Task::categoryToString(task.category))));
    }

    // ★★★ 表格居中 ★★★
    table->horizontalHeader()->setMinimumSectionSize(40);
    layout->addWidget(table);

    QPushButton *okButton = new QPushButton("确定", dialog);
    okButton->setFixedWidth(70);
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(okButton);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(okButton, &QPushButton::clicked, dialog, &QDialog::accept);

    dialog->exec();
    delete dialog;
}

// ========== 日历功能 ==========

void MainWindow::updateCalendar()
{
    if (!m_taskManager) return;

    QTextCharFormat format;
    format.setBackground(Qt::red);
    format.setForeground(Qt::white);

    auto tasks = m_taskManager->getAllTasks();
    m_calendarWidget->setDateTextFormat(QDate(), QTextCharFormat());
    for (const auto& task : tasks) {
        QDateTime dt = QDateTime::fromSecsSinceEpoch(task.startTime);
        QDate date = dt.date();
        m_calendarWidget->setDateTextFormat(date, format);
    }
}

void MainWindow::onCalendarClicked(const QDate &date)
{
    m_dateEdit->setDate(date);
    loadTasksForToday();
}

// ========== 音频功能 ==========

void MainWindow::playReminderSound()
{
    std::string cmd = "aplay /home/code/Desktop/MySchedule/alert.wav 2>/dev/null &";
    system(cmd.c_str());
}

void MainWindow::stopReminderSound()
{
    if (m_mediaPlayer && m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_mediaPlayer->stop();
        m_isSoundPlaying = false;
    }
}

// ========== 提醒弹窗 ==========

void MainWindow::showReminderPopup(const Task& task)
{
    playReminderSound();

    QString title = "⏰ 任务提醒";
    QString message = QString(
                          "任务名称：%1\n"
                          "开始时间：%2\n"
                          "优先级：%3\n"
                          "分类：%4"
                          ).arg(QString::fromStdString(task.name))
                          .arg(QString::fromStdString(timeToStr(task.startTime)))
                          .arg(QString::fromStdString(Task::priorityToString(task.priority)))
                          .arg(QString::fromStdString(Task::categoryToString(task.category)));

    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setWindowTitle(title);
    msgBox->setText(message);
    msgBox->setIcon(QMessageBox::Information);
    msgBox->setStandardButtons(QMessageBox::Ok);
    msgBox->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    msgBox->setStyleSheet(
        "QMessageBox {"
        "    background-color: #2b2b2b;"
        "    color: #ffffff;"
        "    border: 2px solid #ff6b35;"
        "    border-radius: 10px;"
        "}"
        "QPushButton {"
        "    background-color: #ff6b35;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 20px;"
        "    border-radius: 5px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #ff8c5a;"
        "}"
        "QLabel {"
        "    color: #ffffff;"
        "}"
        );

    connect(msgBox, &QMessageBox::finished, this, [this]() {
        stopReminderSound();
    });

    msgBox->show();

    qDebug().noquote() << QString("[REMINDER] Task \"%1\" starts at %2")
                              .arg(QString::fromStdString(task.name))
                              .arg(QString::fromStdString(timeToStr(task.startTime)));
}

// ========== 账户管理 ==========

void MainWindow::onLoginClicked()
{
    std::string username = m_usernameEdit->text().toStdString();
    std::string password = m_passwordEdit->text().toStdString();

    if (username.empty() || password.empty()) {
        showMessage("错误", "用户名和密码不能为空！", true);
        return;
    }

    if (loginUser(username, password)) {
        m_currentUser = username;
        m_isLoggedIn = true;
        m_taskManager = std::make_unique<TaskManager>(username);

        m_taskManager->setReminderCallback([this](const Task& task) {
            QMetaObject::invokeMethod(this, [this, task]() {
                showReminderPopup(task);
            });
        });

        updateLoginStatus();
        toggleControls(true);
        loadTasksForToday();
        updateCalendar();
        showMessage("成功", QString("欢迎回来，%1！").arg(QString::fromStdString(username)));
        m_passwordEdit->clear();

        if (m_reminderThread.joinable()) {
            m_reminderThread.join();
        }
        m_running = true;
        m_reminderThread = std::thread(&MainWindow::reminderThreadFunc, this);
    } else {
        showMessage("登录失败", "用户名或密码错误！", true);
    }
}

void MainWindow::onRegisterClicked()
{
    std::string username = m_usernameEdit->text().toStdString();
    std::string password = m_passwordEdit->text().toStdString();

    if (username.empty() || password.empty()) {
        showMessage("错误", "用户名和密码不能为空！", true);
        return;
    }

    if (registerUser(username, password)) {
        showMessage("成功", "注册成功！请使用新账户登录。");
        m_passwordEdit->clear();
    } else {
        showMessage("注册失败", "用户名已存在或注册失败！", true);
    }
}

void MainWindow::onLogoutClicked()
{
    m_isLoggedIn = false;
    m_currentUser = "";
    m_taskManager.reset();
    m_running = false;
    if (m_reminderThread.joinable()) {
        m_reminderThread.join();
    }
    updateLoginStatus();
    toggleControls(false);
    m_taskTableWidget->setRowCount(0);
    m_taskCountLabel->setText("任务数：0");
    m_calendarWidget->setDateTextFormat(QDate(), QTextCharFormat());
    setWindowTitle("MySchedule - 智能日程管理");
    showMessage("提示", "已安全退出登录。");
}

// ========== 任务管理 ==========

void MainWindow::onAddTaskClicked()
{
    if (!m_isLoggedIn || !m_taskManager) {
        showMessage("错误", "请先登录！", true);
        return;
    }

    std::string taskName = m_taskNameEdit->text().toStdString();
    if (taskName.empty()) {
        showMessage("错误", "任务名称不能为空！", true);
        return;
    }

    QDateTime dateTime(m_dateEdit->date(), m_timeEdit->time());
    time_t startTime = dateTime.toSecsSinceEpoch();
    std::string priority = m_priorityComboBox->currentText().toStdString();
    std::string category = m_categoryComboBox->currentText().toStdString();

    int remindMinutes = m_remindMinutesSpinBox->value();
    time_t remindTime;
    if (remindMinutes > 0) {
        remindTime = startTime - (remindMinutes * 60);
    } else {
        remindTime = 0;
    }

    if (m_taskManager->addTask(taskName, startTime, priority, category, remindTime)) {
        QString successMsg;
        if (remindMinutes > 0) {
            successMsg = QString("任务已添加！将在 %1 分钟后提醒").arg(remindMinutes);
        } else {
            successMsg = "任务已添加！（无提醒）";
        }
        showMessage("成功", successMsg);
        m_taskNameEdit->clear();
        loadTasksForToday();
        updateCalendar();
    } else {
        showMessage("失败", "该开始时间已被占用！", true);
    }
}

void MainWindow::onDeleteTaskClicked()
{
    if (!m_isLoggedIn || !m_taskManager) {
        showMessage("错误", "请先登录！", true);
        return;
    }

    int currentRow = m_taskTableWidget->currentRow();
    if (currentRow < 0) {
        showMessage("提示", "请先选择要删除的任务。", true);
        return;
    }

    int taskId = m_taskTableWidget->item(currentRow, 0)->text().toInt();

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除",
                                  QString("删除任务 ID %1？").arg(taskId),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (m_taskManager->deleteTask(taskId)) {
            showMessage("成功", "任务已删除！");
            loadTasksForToday();
            updateCalendar();
        } else {
            showMessage("失败", "删除失败！", true);
        }
    }
}

// ========== 修改任务功能 ==========

void MainWindow::onEditTaskClicked()
{
    if (!m_isLoggedIn || !m_taskManager) {
        showMessage("错误", "请先登录！", true);
        return;
    }

    int currentRow = m_taskTableWidget->currentRow();
    if (currentRow < 0) {
        showMessage("提示", "请先选择要修改的任务。", true);
        return;
    }

    int taskId = m_taskTableWidget->item(currentRow, 0)->text().toInt();
    auto allTasks = m_taskManager->getAllTasks();
    Task* targetTask = nullptr;
    for (auto& task : allTasks) {
        if (task.id == taskId) {
            targetTask = &task;
            break;
        }
    }

    if (!targetTask) {
        showMessage("错误", "任务不存在！", true);
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QString("修改任务 ID: %1").arg(taskId));

    QFormLayout *form = new QFormLayout(&dialog);
    QLineEdit *nameEdit = new QLineEdit(QString::fromStdString(targetTask->name));
    QDateEdit *dateEdit = new QDateEdit(QDateTime::fromSecsSinceEpoch(targetTask->startTime).date());
    dateEdit->setCalendarPopup(true);
    QTimeEdit *timeEdit = new QTimeEdit(QDateTime::fromSecsSinceEpoch(targetTask->startTime).time());
    QComboBox *priorityBox = new QComboBox;
    priorityBox->addItems({"高", "中", "低"});
    priorityBox->setCurrentText(QString::fromStdString(Task::priorityToString(targetTask->priority)));
    QComboBox *categoryBox = new QComboBox;
    categoryBox->addItems({"学习", "娱乐", "生活"});
    categoryBox->setCurrentText(QString::fromStdString(Task::categoryToString(targetTask->category)));
    QSpinBox *remindSpin = new QSpinBox;
    remindSpin->setRange(0, 60);
    remindSpin->setValue(5);
    remindSpin->setSuffix(" 分钟");

    form->addRow("任务名称：", nameEdit);
    form->addRow("日期：", dateEdit);
    form->addRow("时间：", timeEdit);
    form->addRow("优先级：", priorityBox);
    form->addRow("分类：", categoryBox);
    form->addRow("提前提醒：", remindSpin);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        if (!m_taskManager->deleteTask(taskId)) {
            showMessage("错误", "修改失败！", true);
            return;
        }

        QDateTime dateTime(dateEdit->date(), timeEdit->time());
        time_t startTime = dateTime.toSecsSinceEpoch();
        std::string priority = priorityBox->currentText().toStdString();
        std::string category = categoryBox->currentText().toStdString();
        int remindMinutes = remindSpin->value();
        time_t remindTime = remindMinutes > 0 ? startTime - remindMinutes * 60 : 0;

        if (m_taskManager->addTask(nameEdit->text().toStdString(), startTime, priority, category, remindTime)) {
            showMessage("成功", "任务修改成功！");
            loadTasksForToday();
            updateCalendar();
        } else {
            showMessage("错误", "修改失败，时间可能被占用！", true);
        }
    }
}

void MainWindow::onRefreshClicked()
{
    loadTasksForToday();
}

void MainWindow::onShowAllClicked()
{
    if (!m_isLoggedIn || !m_taskManager) {
        showMessage("错误", "请先登录！", true);
        return;
    }

    auto tasks = m_taskManager->getAllTasks();
    QString allTasks;

    if (tasks.empty()) {
        allTasks = "暂无任务。";
    } else {
        for (const auto& task : tasks) {
            allTasks += QString("ID: %1 | %2 | %3 | %4 | %5\n")
            .arg(task.id)
                .arg(QString::fromStdString(task.name))
                .arg(QString::fromStdString(timeToStr(task.startTime)))
                .arg(QString::fromStdString(Task::priorityToString(task.priority)))
                .arg(QString::fromStdString(Task::categoryToString(task.category)));
        }
    }

    QMessageBox::information(this, "所有任务", allTasks);
}

void MainWindow::onDateChanged(const QDate &date)
{
    Q_UNUSED(date);
    if (m_isLoggedIn) loadTasksForToday();
}

void MainWindow::onReminderTimer()
{
    if (m_taskManager) {
        m_taskManager->checkReminders();
    }
}

void MainWindow::reminderThreadFunc()
{
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// ========== 语音识别（带进度弹窗） ==========

void MainWindow::onVoiceInputClicked()
{
    // 创建进度对话框
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("语音识别");
    dialog->setModal(false);
    dialog->setFixedSize(350, 200);
    dialog->setStyleSheet("QDialog { background-color: white; border: 2px solid #9C27B0; border-radius: 10px; }");
    dialog->show();

    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->setSpacing(15);

    QLabel *statusLabel = new QLabel("录音中...");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("font-size: 20px; padding: 10px; color: #9C27B0;");
    layout->addWidget(statusLabel);

    QLabel *countdownLabel = new QLabel("5");
    countdownLabel->setAlignment(Qt::AlignCenter);
    countdownLabel->setStyleSheet("font-size: 52px; font-weight: bold; color: #9C27B0;");
    layout->addWidget(countdownLabel);

    QProgressBar *progressBar = new QProgressBar(dialog);
    progressBar->setRange(0, 5);
    progressBar->setValue(5);
    progressBar->setTextVisible(false);
    progressBar->setStyleSheet(
        "QProgressBar { border-radius: 5px; background-color: #e0e0e0; height: 6px; }"
        "QProgressBar::chunk { background-color: #9C27B0; border-radius: 5px; }"
    );
    layout->addWidget(progressBar);

    QPushButton *cancelButton = new QPushButton("取消");
    cancelButton->setStyleSheet("QPushButton { padding: 6px 20px; }");
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(cancelButton);
    layout->addLayout(btnLayout);

    // 取消按钮关闭对话框
    connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::reject);

    // ★★★ 用 QTimer 更新倒计时 ★★★
    QTimer *timer = new QTimer(dialog);
    int seconds = 5;
    timer->start(1000);

    connect(timer, &QTimer::timeout, [=]() mutable {
        seconds--;
        countdownLabel->setText(QString::number(seconds));
        progressBar->setValue(seconds);
        if (seconds <= 0) {
            timer->stop();
            statusLabel->setText("识别中...");
            statusLabel->setStyleSheet("font-size: 20px; padding: 10px; color: #FF9800;");
            countdownLabel->setText("⏳");
            countdownLabel->setStyleSheet("font-size: 32px; color: #FF9800;");
            progressBar->setRange(0, 0);
            progressBar->setValue(0);
        }
    });

    // 用线程执行录音，不阻塞界面
    std::thread([=]() {
        system("arecord -d 5 -r 16000 -c 1 -f S16_LE -t wav /tmp/voice_input.wav 2>/dev/null");

        // 识别
        QProcess process;
        process.start("python3", {"/home/code/Desktop/MySchedule/qt/voice_rec.py"});
        if (process.waitForFinished(10000)) {
            QString result = process.readAllStandardOutput().trimmed();

            QMetaObject::invokeMethod(this, [=]() {
                if (!result.isEmpty()) {
                    m_taskNameEdit->setText(result);
                    statusLabel->setText("✅ 识别成功！");
                    statusLabel->setStyleSheet("font-size: 20px; padding: 10px; color: green;");
                    countdownLabel->setText("✅");
                    countdownLabel->setStyleSheet("font-size: 32px; color: green;");
                    progressBar->setRange(0, 1);
                    progressBar->setValue(1);
                } else {
                    statusLabel->setText("❌ 识别失败");
                    statusLabel->setStyleSheet("font-size: 20px; padding: 10px; color: red;");
                    countdownLabel->setText("❌");
                    countdownLabel->setStyleSheet("font-size: 32px; color: red;");
                }
                QTimer::singleShot(1500, dialog, &QDialog::accept);
            });
        } else {
            process.kill();
            QMetaObject::invokeMethod(this, [=]() {
                statusLabel->setText("❌ 识别超时");
                statusLabel->setStyleSheet("font-size: 20px; padding: 10px; color: red;");
                QTimer::singleShot(1500, dialog, &QDialog::accept);
            });
        }
    }).detach();
}

// ========== 辅助函数 ==========

void MainWindow::updateLoginStatus()
{
    if (m_isLoggedIn) {
        m_loginStatusLabel->setText(QString("已登录：%1").arg(QString::fromStdString(m_currentUser)));
        m_loginStatusLabel->setStyleSheet("color: green;");
    } else {
        m_loginStatusLabel->setText("未登录");
        m_loginStatusLabel->setStyleSheet("color: red;");
    }
}

void MainWindow::toggleControls(bool enabled)
{
    m_usernameEdit->setEnabled(!enabled);
    m_passwordEdit->setEnabled(!enabled);
    m_loginButton->setEnabled(!enabled);
    m_registerButton->setEnabled(!enabled);
    m_logoutButton->setEnabled(enabled);
    m_taskNameEdit->setEnabled(enabled);
    m_dateEdit->setEnabled(enabled);
    m_timeEdit->setEnabled(enabled);
    m_priorityComboBox->setEnabled(enabled);
    m_categoryComboBox->setEnabled(enabled);
    m_remindMinutesSpinBox->setEnabled(enabled);
    m_addTaskButton->setEnabled(enabled);
    m_voiceButton->setEnabled(enabled);
    m_deleteTaskButton->setEnabled(enabled);
    m_editTaskButton->setEnabled(enabled);
    m_refreshButton->setEnabled(enabled);
    m_showAllButton->setEnabled(enabled);
    m_yearComboBox->setEnabled(enabled);
    m_monthComboBox->setEnabled(enabled);
    m_showMonthButton->setEnabled(enabled);
    m_taskTableWidget->setEnabled(enabled);
}

void MainWindow::showMessage(const QString& title, const QString& message, bool isError)
{
    if (isError) {
        QMessageBox::critical(this, title, message);
    } else {
        QMessageBox::information(this, title, message);
    }
}

void MainWindow::loadTasksForToday()
{
    if (!m_isLoggedIn || !m_taskManager) return;

    QDateTime dateTime(m_dateEdit->date(), QTime(0, 0, 0));
    time_t dateTimeT = dateTime.toSecsSinceEpoch();
    auto tasks = m_taskManager->getTasksForDay(dateTimeT);
    updateTaskTable(tasks);

    setWindowTitle(QString("MySchedule - %1 的任务")
                       .arg(QString::fromStdString(m_currentUser)));
}

void MainWindow::updateTaskTable(const std::vector<Task>& tasks)
{
    m_taskTableWidget->setRowCount(0);
    m_taskTableWidget->setRowCount(tasks.size());

    for (size_t i = 0; i < tasks.size(); ++i) {
        const auto& task = tasks[i];
        m_taskTableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(task.id)));
        m_taskTableWidget->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(task.name)));
        m_taskTableWidget->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(timeToStr(task.startTime))));
        m_taskTableWidget->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(Task::priorityToString(task.priority))));
        m_taskTableWidget->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(Task::categoryToString(task.category))));
        m_taskTableWidget->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(timeToStr(task.remindTime))));
    }
}

