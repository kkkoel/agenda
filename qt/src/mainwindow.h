#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QDateEdit>
#include <QTimeEdit>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QProgressBar>
#include <QSpinBox>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QCalendarWidget>
#include <QProcess>
#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include "TaskManager.h"
#include <QProcess>
#include <QTableWidget>
#include <QTableWidgetItem>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onLogoutClicked();
    void onAddTaskClicked();
    void onDeleteTaskClicked();
    void onEditTaskClicked();
    void onRefreshClicked();
    void onShowAllClicked();
    void onDateChanged(const QDate &date);
    void onReminderTimer();
    void onShowMonthClicked();
    void onCalendarClicked(const QDate &date);
    void onVoiceInputClicked();

private:
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QPushButton *m_loginButton;
    QPushButton *m_registerButton;
    QPushButton *m_logoutButton;
    QLabel *m_loginStatusLabel;

    QLineEdit *m_taskNameEdit;
    QDateEdit *m_dateEdit;
    QTimeEdit *m_timeEdit;
    QComboBox *m_priorityComboBox;
    QComboBox *m_categoryComboBox;
    QPushButton *m_addTaskButton;
    QPushButton *m_voiceButton;
    QSpinBox *m_remindMinutesSpinBox;

    QTableWidget *m_taskTableWidget;
    QPushButton *m_refreshButton;
    QPushButton *m_showAllButton;
    QPushButton *m_deleteTaskButton;
    QPushButton *m_editTaskButton;

    QComboBox *m_monthComboBox;
    QComboBox *m_yearComboBox;
    QPushButton *m_showMonthButton;
    QLabel *m_taskCountLabel;

    QCalendarWidget *m_calendarWidget;

    QMediaPlayer *m_mediaPlayer;
    QAudioOutput *m_audioOutput;
    bool m_isSoundPlaying;
    QString m_soundFilePath;

    std::unique_ptr<TaskManager> m_taskManager;
    std::string m_currentUser;
    bool m_isLoggedIn;
    std::atomic<bool> m_running;
    std::thread m_reminderThread;
    QTimer *m_timer;

    void updateLoginStatus();
    void updateTaskTable(const std::vector<Task>& tasks);
    void loadTasksForToday();
    void loadTasksForMonth(int year, int month);
    void showMessage(const QString& title, const QString& message, bool isError = false);
    void toggleControls(bool enabled);
    void reminderThreadFunc();
    void showReminderPopup(const Task& task);
    void playReminderSound();
    void stopReminderSound();
    void updateCalendar();

    std::vector<Task> getTasksForMonth(int year, int month) const;
    QString getMonthName(int month) const;
};

#endif
