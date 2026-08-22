#ifndef CMI_UI_SEGMENTEDCONTROL_H
#define CMI_UI_SEGMENTEDCONTROL_H

#include <QWidget>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVariant>
#include <vector>

namespace cmi {

class SegmentedControl : public QWidget {
    Q_OBJECT
public:
    explicit SegmentedControl(QWidget *parent = nullptr);

    void addSegment(const QString &text, const QVariant &data = QVariant());
    void setCurrentIndex(int index);
    int currentIndex() const;
    QVariant currentData() const;
    QString currentText() const;
    void clear();

signals:
    void currentIndexChanged(int index);
    void currentDataChanged(const QVariant &data);

private:
    QHBoxLayout *m_layout = nullptr;
    QButtonGroup *m_group = nullptr;
    std::vector<QVariant> m_segmentData;
};

} // namespace cmi

#endif // CMI_UI_SEGMENTEDCONTROL_H
