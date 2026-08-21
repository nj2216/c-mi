#include "ControlPanel.h"

#include <QSettings>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

namespace cmi {

namespace {
int xioctl(int fd, unsigned long request, void *arg)
{
    int r;
    do {
        r = ::ioctl(fd, request, arg);
    } while (r == -1 && errno == EINTR);
    return r;
}
} // namespace

ControlPanel::ControlPanel(QObject *parent) : QObject(parent) {}

ControlPanel::~ControlPanel()
{
    close();
}

bool ControlPanel::open(const QString &node)
{
    close();
    m_fd = ::open(node.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK);
    if (m_fd < 0)
        return false;
    enumerateControls();
    refreshAll();
    return true;
}

void ControlPanel::close()
{
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    m_controls.clear();
}

void ControlPanel::enumerateControls()
{
    m_controls.clear();

    struct v4l2_queryctrl qc;
    memset(&qc, 0, sizeof(qc));
    qc.id = V4L2_CID_USER_CLASS | V4L2_CTRL_FLAG_NEXT_CTRL;

    while (xioctl(m_fd, VIDIOC_QUERYCTRL, &qc) == 0) {
        if (!(qc.flags & V4L2_CTRL_FLAG_DISABLED) &&
            (qc.type == V4L2_CTRL_TYPE_INTEGER ||
             qc.type == V4L2_CTRL_TYPE_BOOLEAN ||
             qc.type == V4L2_CTRL_TYPE_MENU ||
             qc.type == V4L2_CTRL_TYPE_INTEGER_MENU)) {
            Control c;
            c.id = qc.id;
            c.name = QString::fromLatin1(reinterpret_cast<const char *>(qc.name)).trimmed();
            c.minimum = qc.minimum;
            c.maximum = qc.maximum;
            c.step = qc.step > 0 ? qc.step : 1;
            c.defaultValue = qc.default_value;
            c.isMenu = (qc.type == V4L2_CTRL_TYPE_MENU ||
                        qc.type == V4L2_CTRL_TYPE_INTEGER_MENU);
            c.isBoolean = (qc.type == V4L2_CTRL_TYPE_BOOLEAN);
            if (c.isMenu)
                enumerateMenu(c);
            m_controls.push_back(c);
        }
        qc.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }
}

void ControlPanel::enumerateMenu(Control &ctrl)
{
    struct v4l2_querymenu qm;
    memset(&qm, 0, sizeof(qm));
    qm.id = ctrl.id;
    for (qm.index = ctrl.minimum; qm.index <= ctrl.maximum; ++qm.index) {
        if (xioctl(m_fd, VIDIOC_QUERYMENU, &qm) == 0) {
            ctrl.menuItems.append({
                static_cast<int32_t>(qm.index),
                QString::fromLatin1(reinterpret_cast<const char *>(qm.name)).trimmed()
            });
        }
    }
}

const ControlPanel::Control *ControlPanel::controlById(uint32_t id) const
{
    for (const Control &c : m_controls) {
        if (c.id == id)
            return &c;
    }
    return nullptr;
}

bool ControlPanel::refreshValue(uint32_t id)
{
    if (m_fd < 0)
        return false;

    struct v4l2_control ctrl;
    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = id;
    if (xioctl(m_fd, VIDIOC_G_CTRL, &ctrl) < 0)
        return false;

    for (Control &c : m_controls) {
        if (c.id == id) {
            c.value = ctrl.value;
            return true;
        }
    }
    return false;
}

void ControlPanel::refreshAll()
{
    for (Control &c : m_controls)
        refreshValue(c.id);
}

bool ControlPanel::setValue(uint32_t id, int32_t value)
{
    if (m_fd < 0)
        return false;

    struct v4l2_control ctrl;
    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = id;
    ctrl.value = value;

    if (xioctl(m_fd, VIDIOC_S_CTRL, &ctrl) < 0)
        return false;

    for (Control &c : m_controls) {
        if (c.id == id) {
            c.value = ctrl.value;
            break;
        }
    }
    emit controlChanged(id, ctrl.value);
    return true;
}

void ControlPanel::savePreset(const QString &deviceKey, const QString &presetName) const
{
    QSettings s(QStringLiteral("c-mi"), QStringLiteral("c-mi"));
    s.beginGroup(QStringLiteral("presets/%1/%2").arg(deviceKey, presetName));
    for (const Control &c : m_controls)
        s.setValue(QString::number(c.id), c.value);
    s.endGroup();
}

void ControlPanel::loadPreset(const QString &deviceKey, const QString &presetName)
{
    QSettings s(QStringLiteral("c-mi"), QStringLiteral("c-mi"));
    s.beginGroup(QStringLiteral("presets/%1/%2").arg(deviceKey, presetName));
    const QStringList keys = s.childKeys();
    for (const QString &key : keys) {
        bool ok = false;
        uint32_t id = key.toUInt(&ok);
        if (ok)
            setValue(id, s.value(key).toInt());
    }
    s.endGroup();
}

QStringList ControlPanel::presets(const QString &deviceKey)
{
    QSettings s(QStringLiteral("c-mi"), QStringLiteral("c-mi"));
    s.beginGroup(QStringLiteral("presets/%1").arg(deviceKey));
    const QStringList list = s.childGroups();
    s.endGroup();
    return list;
}

} // namespace cmi
