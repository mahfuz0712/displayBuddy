#pragma once

#include <QMainWindow>
#include <QMap>
#include <QStringList>
#include <QVector>

#include "brightnessbackend.h"
#include "theme.h"

class QLabel;
class QMenu;
class QPushButton;
class QScrollArea;
class QSystemTrayIcon;
class QThread;
class QToolButton;
class QVBoxLayout;
class BrightnessWorker;
class ClickSlider;
class FlowLayout;
class PresetCard;
class ResponsiveGrid;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // True when a system tray icon is shown (the window then hides to the tray instead of closing).
    bool hasTray() const { return m_tray != nullptr; }

public slots:
    // Brings the window back from the tray / minimized state and focuses it.
    void showFromTray();
    // Really exits the app (closing the window only hides it to the tray).
    void quitApp();

signals:
    void requestScan();
    void requestApply(const QString& id, int value);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void refreshDisplays();
    void onScanned(const QVector<DisplayInfo>& displays);
    void onApplied(const QString& id, int value, bool ok, const QString& error);
    void showSettingsMenu();

private:
    struct DisplayCard {
        QString id;
        QString name;
        bool controllable = false;
        QLabel* value = nullptr;
        ClickSlider* slider = nullptr;
    };

    // theme / chrome
    void applyTheme();
    void applyWindowChrome();

    // system tray
    void setupTray();
    void saveWindowGeometry();

    // presets
    void loadCustomPresets();
    void saveCustomPresets() const;
    void rebuildPresets();
    void updatePresetHighlight();
    void applyPreset(const QString& name, int value);
    void createCustomPreset();
    void removeCustomPreset(const QString& name);
    int currentBrightnessGuess() const;

    // displays
    void buildDisplayCards();
    QWidget* makeDisplayCard(int index);
    QWidget* makeEmptyCard();
    void setDisplayBrightness(int index, int value);
    void queueApply(const QString& id, int value);

    void setStatus(const QString& text, bool error = false);

    Theme m_theme;
    QVBoxLayout* m_root = nullptr;
    QLabel* m_logo = nullptr;
    QToolButton* m_refreshButton = nullptr;
    QToolButton* m_settingsButton = nullptr;
    QWidget* m_presetHost = nullptr;
    FlowLayout* m_presetFlow = nullptr;
    QLabel* m_count = nullptr;
    QLabel* m_status = nullptr;
    ResponsiveGrid* m_grid = nullptr;

    QVector<DisplayInfo> m_displays;
    QVector<DisplayCard> m_cards;
    QMap<QString, int> m_customPresets;
    QString m_activePreset;

    QThread* m_thread = nullptr;
    BrightnessWorker* m_worker = nullptr;
    bool m_scanning = false;
    int m_pending = 0;
    QString m_okMessage;
    QStringList m_errors;
    int m_marginMode = -1;
    bool m_chromeApplied = false;

    QSystemTrayIcon* m_tray = nullptr;
    QMenu* m_trayMenu = nullptr;
    bool m_quitting = false;
    bool m_trayHintShown = false;
};