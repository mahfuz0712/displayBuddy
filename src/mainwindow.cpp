#include "mainwindow.h"

#include "brightnessworker.h"
#include "flowlayout.h"
#include "icons.h"
#include "widgets.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSlider>
#include <QStyle>
#include <QStyleHints>
#include <QThread>
#include <QToolButton>
#include <QVBoxLayout>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>
#endif

namespace
{

    struct BuiltinPreset
    {
        const char *name;
        int value;
    };
    constexpr BuiltinPreset kBuiltinPresets[] = {{"Designing", 85}, {"Coding", 65}, {"Editing", 95}};

    bool isBuiltinName(const QString &name)
    {
        for (const auto &p : kBuiltinPresets)
            if (name.compare(QString::fromLatin1(p.name), Qt::CaseInsensitive) == 0)
                return true;
        return false;
    }

    QSettings appSettings() { return QSettings("DisplayBuddy", "DisplayBuddy"); }

    QLabel *iconLabel(Icons::Kind kind, const QColor &color, int size, qreal dpr)
    {
        auto *l = new QLabel;
        l->setPixmap(Icons::pixmap(kind, color, size, dpr));
        l->setFixedSize(size, size);
        l->setAttribute(Qt::WA_TranslucentBackground);
        return l;
    }

    // Small modal dialog: preset name + brightness.
    class PresetDialog : public QDialog
    {
    public:
        PresetDialog(int initial, QWidget *parent) : QDialog(parent)
        {
            setWindowTitle("New preset");
            setModal(true);
            setMinimumWidth(320);

            auto *lay = new QVBoxLayout(this);
            lay->setContentsMargins(20, 18, 20, 16);
            lay->setSpacing(10);

            auto *nameTitle = new QLabel("Preset name");
            nameTitle->setObjectName("sectionTitle");
            lay->addWidget(nameTitle);
            m_name = new QLineEdit;
            m_name->setMaxLength(18);
            m_name->setPlaceholderText("e.g. Reading");
            lay->addWidget(m_name);

            auto *row = new QHBoxLayout;
            auto *briTitle = new QLabel("Brightness");
            briTitle->setObjectName("sectionTitle");
            m_value = new QLabel;
            m_value->setObjectName("brightnessValue");
            row->addWidget(briTitle);
            row->addStretch();
            row->addWidget(m_value);
            lay->addLayout(row);

            m_slider = new ClickSlider;
            m_slider->setValueQuiet(initial);
            m_value->setText(QString::number(initial) + "%");
            connect(m_slider, &QSlider::valueChanged, this, [this](int v)
                    { m_value->setText(QString::number(v) + "%"); });
            lay->addWidget(m_slider);

            m_buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
            m_buttons->button(QDialogButtonBox::Save)->setEnabled(false);
            lay->addSpacing(6);
            lay->addWidget(m_buttons);
            connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
            connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
            connect(m_name, &QLineEdit::textChanged, this, [this](const QString &t)
                    { m_buttons->button(QDialogButtonBox::Save)->setEnabled(!t.trimmed().isEmpty()); });
            connect(m_name, &QLineEdit::returnPressed, this, [this]
                    {
            if (!m_name->text().trimmed().isEmpty()) accept(); });
            m_name->setFocus();
        }

        QString presetName() const { return m_name->text().trimmed(); }
        int presetValue() const { return m_slider->value(); }

    private:
        QLineEdit *m_name = nullptr;
        QLabel *m_value = nullptr;
        ClickSlider *m_slider = nullptr;
        QDialogButtonBox *m_buttons = nullptr;
    };

} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("DisplayBuddy");
    setWindowIcon(Icons::appIcon());
    setMinimumSize(340, 420);
    resize(520, 700);

    // ---- worker thread (all slow hardware access happens there) ----
    m_thread = new QThread(this);
    m_worker = new BrightnessWorker;
    m_worker->moveToThread(m_thread);
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &MainWindow::requestScan, m_worker, &BrightnessWorker::scan);
    connect(this, &MainWindow::requestApply, m_worker, &BrightnessWorker::apply);
    connect(m_worker, &BrightnessWorker::scanned, this, &MainWindow::onScanned);
    connect(m_worker, &BrightnessWorker::applied, this, &MainWindow::onApplied);
    m_thread->start();

    // ---- layout ----
    auto *central = new QWidget(this);
    central->setObjectName("central");
    m_root = new QVBoxLayout(central);
    m_root->setContentsMargins(20, 12, 20, 14);
    m_root->setSpacing(14);

    auto *header = new QHBoxLayout;
    header->setSpacing(10);
    m_logo = new QLabel;
    m_logo->setFixedSize(28, 28);
    header->addWidget(m_logo);
    auto *title = new QLabel("DisplayBuddy");
    title->setObjectName("title");
    header->addWidget(title);
    header->addStretch();

    m_refreshButton = new QToolButton;
    m_refreshButton->setObjectName("toolButton");
    m_refreshButton->setToolTip("Refresh displays");
    m_refreshButton->setFixedSize(36, 36);
    m_refreshButton->setIconSize(QSize(20, 20));
    m_refreshButton->setCursor(Qt::PointingHandCursor);
    header->addWidget(m_refreshButton);

    m_settingsButton = new QToolButton;
    m_settingsButton->setObjectName("toolButton");
    m_settingsButton->setToolTip("Settings");
    m_settingsButton->setFixedSize(36, 36);
    m_settingsButton->setIconSize(QSize(20, 20));
    m_settingsButton->setCursor(Qt::PointingHandCursor);
    header->addWidget(m_settingsButton);
    m_root->addLayout(header);

    auto *presetTitle = new QLabel("Presets");
    presetTitle->setObjectName("sectionTitle");
    m_root->addWidget(presetTitle);

    m_presetHost = new QWidget;
    m_presetFlow = new FlowLayout(m_presetHost, 0, 10, 10);
    m_root->addWidget(m_presetHost);

    m_count = new QLabel("Displays");
    m_count->setObjectName("sectionTitle");
    m_root->addWidget(m_count);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_grid = new ResponsiveGrid;
    scroll->setWidget(m_grid);
    scroll->viewport()->setAutoFillBackground(false);
    m_grid->setAutoFillBackground(false);
    m_root->addWidget(scroll, 1);

    m_status = new QLabel("Detecting displays…");
    m_status->setObjectName("status");
    m_status->setWordWrap(true);
    m_root->addWidget(m_status);

    setCentralWidget(central);

    connect(m_refreshButton, &QToolButton::clicked, this, &MainWindow::refreshDisplays);
    connect(m_settingsButton, &QToolButton::clicked, this, &MainWindow::showSettingsMenu);

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this]
            { applyTheme(); });
#endif

    QSettings s = appSettings();
    restoreGeometry(s.value("window/geometry").toByteArray());

    loadCustomPresets();
    applyTheme(); // builds icons, presets and (empty) display list
    refreshDisplays();
}

MainWindow::~MainWindow()
{
    m_thread->quit();
    m_thread->wait();
}

// ---------------------------------------------------------------- theme ----

void MainWindow::applyTheme()
{
    m_theme = Theme::make(Theme::systemIsDark());
    qApp->setPalette(m_theme.palette());
    qApp->setStyleSheet(m_theme.styleSheet());

    const qreal dpr = devicePixelRatioF();
    m_logo->setPixmap(Icons::appIcon().pixmap(QSize(28, 28), dpr));
    m_refreshButton->setIcon(Icons::icon(Icons::Kind::Refresh, m_theme.text));
    m_settingsButton->setIcon(Icons::icon(Icons::Kind::Gear, m_theme.text));

    rebuildPresets();
    if (!m_cards.isEmpty() || !m_displays.isEmpty())
        buildDisplayCards();
    applyWindowChrome();
}

void MainWindow::applyWindowChrome()
{
#ifdef Q_OS_WIN
    // Windows 11 look: dark title bar, rounded corners, title bar tinted like the window.
    // Attributes are ignored (harmlessly) on Windows versions that do not know them.
    if (!isVisible() && !testAttribute(Qt::WA_WState_Created))
        return;
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    const BOOL dark = m_theme.dark ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &dark, sizeof(dark));
    const DWORD corner = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, 33 /*DWMWA_WINDOW_CORNER_PREFERENCE*/, &corner, sizeof(corner));
    const COLORREF caption = RGB(m_theme.bg.red(), m_theme.bg.green(), m_theme.bg.blue());
    DwmSetWindowAttribute(hwnd, 35 /*DWMWA_CAPTION_COLOR*/, &caption, sizeof(caption));
    const COLORREF text = RGB(m_theme.text.red(), m_theme.text.green(), m_theme.text.blue());
    DwmSetWindowAttribute(hwnd, 36 /*DWMWA_TEXT_COLOR*/, &text, sizeof(text));
#endif
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (!m_chromeApplied)
    {
        m_chromeApplied = true;
        applyWindowChrome();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings s = appSettings();
    s.setValue("window/geometry", saveGeometry());
    QMainWindow::closeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    const int mode = width() < 430 ? 0 : 1;
    if (mode != m_marginMode)
    {
        m_marginMode = mode;
        const int side = mode == 0 ? 12 : 20;
        m_root->setContentsMargins(side, 12, side, 14);
    }
}

// -------------------------------------------------------------- presets ----

void MainWindow::loadCustomPresets()
{
    m_customPresets.clear();
    QSettings s = appSettings();

    const int n = s.beginReadArray("customPresetList");
    for (int i = 0; i < n; ++i)
    {
        s.setArrayIndex(i);
        const QString name = s.value("name").toString();
        if (!name.isEmpty())
            m_customPresets.insert(name, qBound(0, s.value("value", 70).toInt(), 100));
    }
    s.endArray();

    // Migrate presets saved by DisplayBuddy 1.2.
    if (n == 0)
    {
        const QStringList oldNames = s.value("customPresetNames").toStringList();
        for (const QString &name : oldNames)
            m_customPresets.insert(name, qBound(0, s.value("customPresets/" + name, 70).toInt(), 100));
        if (!oldNames.isEmpty())
            saveCustomPresets();
    }
}

void MainWindow::saveCustomPresets() const
{
    QSettings s = appSettings();
    s.remove("customPresetList");
    s.beginWriteArray("customPresetList", static_cast<int>(m_customPresets.size()));
    int i = 0;
    for (auto it = m_customPresets.cbegin(); it != m_customPresets.cend(); ++it, ++i)
    {
        s.setArrayIndex(i);
        s.setValue("name", it.key());
        s.setValue("value", it.value());
    }
    s.endArray();
    s.remove("customPresetNames");
    s.remove("customPresets");
}

void MainWindow::rebuildPresets()
{
    while (QLayoutItem *item = m_presetFlow->takeAt(0))
    {
        if (QWidget *w = item->widget())
        {
            w->hide();
            w->deleteLater();
        }
        delete item;
    }

    const auto addCard = [this](const QString &name, int value, bool custom)
    {
        auto *card = new PresetCard(name, value, custom, m_presetHost);
        m_presetFlow->addWidget(card);
        connect(card, &QPushButton::clicked, this, [this, name, value]
                { applyPreset(name, value); });
        if (custom)
        {
            card->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(card, &QWidget::customContextMenuRequested, this, [this, card, name](const QPoint &pos)
                    {
                QMenu menu(this);
                QAction* del = menu.addAction(Icons::icon(Icons::Kind::Trash, m_theme.text), "Delete preset");
                if (menu.exec(card->mapToGlobal(pos)) == del) removeCustomPreset(name); });
        }
    };

    for (const auto &p : kBuiltinPresets)
        addCard(QString::fromLatin1(p.name), p.value, false);
    for (auto it = m_customPresets.cbegin(); it != m_customPresets.cend(); ++it)
        addCard(it.key(), it.value(), true);

    auto *add = new QPushButton("New preset", m_presetHost);
    add->setObjectName("presetNew");
    add->setIcon(Icons::icon(Icons::Kind::Plus, m_theme.accent));
    add->setIconSize(QSize(16, 16));
    add->setCursor(Qt::PointingHandCursor);
    add->setMinimumHeight(58);
    add->setToolTip("Save the current brightness as a custom preset");
    connect(add, &QPushButton::clicked, this, &MainWindow::createCustomPreset);
    m_presetFlow->addWidget(add);

    updatePresetHighlight();
    m_presetHost->updateGeometry();
}

void MainWindow::updatePresetHighlight()
{
    const auto cards = m_presetHost->findChildren<PresetCard *>();
    for (PresetCard *c : cards)
        c->setActive(!m_activePreset.isEmpty() && c->presetName() == m_activePreset);
}

int MainWindow::currentBrightnessGuess() const
{
    for (const auto &card : m_cards)
        if (card.controllable && card.slider)
            return card.slider->value();
    return 70;
}

void MainWindow::applyPreset(const QString &name, int value)
{
    int count = 0;
    for (int i = 0; i < m_cards.size(); ++i)
    {
        DisplayCard &card = m_cards[i];
        if (!card.controllable)
            continue;
        card.slider->setValueQuiet(value);
        card.value->setText(QString::number(value) + "%");
        m_displays[i].brightness = value;
        queueApply(card.id, value);
        ++count;
    }
    if (count == 0)
    {
        setStatus("No controllable display is available for this preset.", true);
        return;
    }
    m_activePreset = name;
    updatePresetHighlight();
    m_okMessage = QString("%1 preset applied to %2 display%3.").arg(name).arg(count).arg(count == 1 ? "" : "s");
    setStatus(QString("Applying %1…").arg(name));
}

void MainWindow::createCustomPreset()
{
    if (m_displays.isEmpty())
    {
        setStatus("Connect a controllable display first.", true);
        return;
    }
    PresetDialog dialog(currentBrightnessGuess(), this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString name = dialog.presetName();
    if (name.isEmpty())
        return;
    if (isBuiltinName(name))
    {
        setStatus(QString("“%1” is a built-in preset name. Choose another name.").arg(name), true);
        return;
    }
    m_customPresets.insert(name, dialog.presetValue());
    saveCustomPresets();
    m_activePreset.clear();
    rebuildPresets();
    setStatus(QString("Preset “%1” saved at %2%.").arg(name).arg(dialog.presetValue()));
}

void MainWindow::removeCustomPreset(const QString &name)
{
    if (!m_customPresets.remove(name))
        return;
    saveCustomPresets();
    if (m_activePreset == name)
        m_activePreset.clear();
    rebuildPresets();
    setStatus(QString("Preset “%1” deleted.").arg(name));
}

void MainWindow::showSettingsMenu()
{
    QMenu menu(this);
    QAction *save = menu.addAction(Icons::icon(Icons::Kind::Plus, m_theme.text), "Save current brightness as preset…");
    QMenu *del = menu.addMenu(Icons::icon(Icons::Kind::Trash, m_theme.text), "Delete custom preset");
    if (m_customPresets.isEmpty())
    {
        del->addAction("No custom presets saved")->setEnabled(false);
    }
    else
    {
        for (auto it = m_customPresets.cbegin(); it != m_customPresets.cend(); ++it)
        {
            const QString name = it.key();
            del->addAction(QString("%1  ·  %2%").arg(name).arg(it.value()), this,
                           [this, name]
                           { removeCustomPreset(name); });
        }
    }
    menu.addSeparator();
    QAction *refresh = menu.addAction(Icons::icon(Icons::Kind::Refresh, m_theme.text), "Refresh displays");
    QAction *about = menu.addAction("About DisplayBuddy");

    QAction *chosen = menu.exec(m_settingsButton->mapToGlobal(QPoint(m_settingsButton->width() - menu.sizeHint().width(),
                                                                     m_settingsButton->height() + 2)));
    if (chosen == save)
        createCustomPreset();
    else if (chosen == refresh)
        refreshDisplays();
    else if (chosen == about)
        QMessageBox::about(this, "About DisplayBuddy",
                           QString("<b>DisplayBuddy %1</b><br>Display brightness for every connected monitor.<br><br>"
                                   "MIT License")
                               .arg(QApplication::applicationVersion()));
}

// -------------------------------------------------------------- displays ----

void MainWindow::refreshDisplays()
{
    if (m_scanning)
        return;
    m_scanning = true;
    m_refreshButton->setEnabled(false);
    setStatus("Detecting displays…");
    emit requestScan();
}

void MainWindow::onScanned(const QVector<DisplayInfo> &displays)
{
    m_scanning = false;
    m_refreshButton->setEnabled(true);
    m_displays = displays;
    buildDisplayCards();

    if (m_displays.isEmpty())
    {
        m_count->setText("Displays");
        setStatus("No displays found. Check DDC/CI support, then refresh.", true);
    }
    else
    {
        m_count->setText(QString("Displays  ·  %1").arg(m_displays.size()));
        setStatus("Brightness changes when you release a slider.");
    }
}

void MainWindow::buildDisplayCards()
{
    m_cards.clear();
    QVector<QWidget *> items;
    if (m_displays.isEmpty())
    {
        items << makeEmptyCard();
    }
    else
    {
        m_cards.resize(m_displays.size());
        for (int i = 0; i < m_displays.size(); ++i)
            items << makeDisplayCard(i);
    }
    m_grid->setItems(items);
}

QWidget *MainWindow::makeEmptyCard()
{
    auto *card = new QFrame;
    card->setObjectName("displayCard");
    auto *lay = new QVBoxLayout(card);
    lay->setContentsMargins(20, 22, 20, 22);
    lay->setSpacing(8);
    lay->setAlignment(Qt::AlignCenter);
    auto *icon = iconLabel(Icons::Kind::Monitor, m_theme.subtext, 32, devicePixelRatioF());
    lay->addWidget(icon, 0, Qt::AlignHCenter);
    auto *text = new QLabel("No displays detected");
    text->setObjectName("sectionTitle");
    text->setAlignment(Qt::AlignCenter);
    lay->addWidget(text);
    auto *hint = new QLabel("Connect a monitor and enable DDC/CI in its on-screen menu, then press refresh.");
    hint->setObjectName("muted");
    hint->setWordWrap(true);
    hint->setAlignment(Qt::AlignCenter);
    lay->addWidget(hint);
    return card;
}

QWidget *MainWindow::makeDisplayCard(int index)
{
    const DisplayInfo &d = m_displays[index];
    const qreal dpr = devicePixelRatioF();

    auto *card = new QFrame;
    card->setObjectName("displayCard");
    auto *lay = new QVBoxLayout(card);
    lay->setContentsMargins(16, 14, 16, 14);
    lay->setSpacing(12);

    auto *top = new QHBoxLayout;
    top->setSpacing(10);
    top->addWidget(iconLabel(Icons::Kind::Monitor, d.controllable ? m_theme.text : m_theme.subtext, 22, dpr));
    auto *name = new QLabel(d.name);
    name->setTextInteractionFlags(Qt::TextSelectableByMouse);
    name->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    name->setToolTip(d.name);
    top->addWidget(name, 1);
    lay->addLayout(top);

    auto *row = new QHBoxLayout;
    row->setSpacing(10);
    row->addWidget(iconLabel(Icons::Kind::Sun, d.controllable ? m_theme.accent : m_theme.track, 20, dpr));
    auto *slider = new ClickSlider;
    slider->setValueQuiet(d.brightness);
    slider->setEnabled(d.controllable);
    row->addWidget(slider, 1);
    auto *value = new QLabel(d.controllable ? QString::number(d.brightness) + "%" : QString("—"));
    value->setObjectName(d.controllable ? "brightnessValue" : "muted");
    value->setFixedWidth(44);
    value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    row->addWidget(value);
    lay->addLayout(row);

    if (!d.controllable)
    {
        auto *hint = new QLabel("Detected, but brightness control is unavailable. Check DDC/CI in the monitor menu.");
        hint->setObjectName("muted");
        hint->setWordWrap(true);
        lay->addWidget(hint);
    }

    DisplayCard &c = m_cards[index];
    c.id = d.id;
    c.name = d.name;
    c.controllable = d.controllable;
    c.value = value;
    c.slider = slider;

    connect(slider, &QSlider::valueChanged, value, [value](int v)
            { value->setText(QString::number(v) + "%"); });
    connect(slider, &ClickSlider::committed, this, [this, index](int v)
            { setDisplayBrightness(index, v); });
    return card;
}

void MainWindow::setDisplayBrightness(int index, int value)
{
    if (index < 0 || index >= m_displays.size())
        return;
    if (m_displays[index].brightness == value)
        return; // nothing changed
    m_displays[index].brightness = value;
    m_activePreset.clear();
    updatePresetHighlight();
    m_okMessage = QString("%1 set to %2%.").arg(m_displays[index].name).arg(value);
    queueApply(m_displays[index].id, value);
}

void MainWindow::queueApply(const QString &id, int value)
{
    ++m_pending;
    emit requestApply(id, value);
}

void MainWindow::onApplied(const QString &, int, bool ok, const QString &error)
{
    m_pending = qMax(0, m_pending - 1);
    if (!ok && !error.isEmpty() && !m_errors.contains(error))
        m_errors << error;
    if (m_pending > 0)
        return;

    if (!m_errors.isEmpty())
    {
        setStatus(m_errors.join(' '), true);
        m_errors.clear();
    }
    else if (!m_okMessage.isEmpty())
    {
        setStatus(m_okMessage);
    }
}

void MainWindow::setStatus(const QString &text, bool error)
{
    m_status->setText(text);
    if (m_status->property("error").toBool() != error)
    {
        m_status->setProperty("error", error);
        m_status->style()->unpolish(m_status);
        m_status->style()->polish(m_status);
    }
}
