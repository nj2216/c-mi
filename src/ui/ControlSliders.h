#ifndef CMI_UI_CONTROLSLIDERS_H
#define CMI_UI_CONTROLSLIDERS_H

#include <QWidget>

class QVBoxLayout;

namespace cmi {

class ControlPanel;

// Builds sliders / checkboxes / dropdowns for every control a ControlPanel
// exposes, bound live to VIDIOC_S_CTRL. Includes preset save/load via
// QSettings.
class ControlSliders : public QWidget {
    Q_OBJECT
public:
    explicit ControlSliders(QWidget *parent = nullptr);

    void bind(ControlPanel *panel, const QString &deviceKey);
    void clear();

private:
    void rebuild();
    void onSavePreset();
    void onLoadPreset();

    ControlPanel *m_panel = nullptr;
    QString m_deviceKey;
    QVBoxLayout *m_layout = nullptr;
};

} // namespace cmi

#endif // CMI_UI_CONTROLSLIDERS_H
