#pragma once

#include "shvcoreqtglobal.h"

#include <shv/core/log.h>

#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QVariant>

inline NecroLog &operator<<(NecroLog log, const QString &s) { return log.operator <<(s.toUtf8().constData()); }
inline NecroLog &operator<<(NecroLog log, const QStringList &s) { return log.operator<<(("(" + s.join(", ") + ")").toStdString()); }
inline NecroLog &operator<<(NecroLog log, const QVariant &v) { return log.operator<<(v.toString().toStdString()); }
SHVCOREQT_DECL_EXPORT NecroLog &operator<<(NecroLog log, const QByteArray &s);
inline NecroLog &operator<<(NecroLog log, const QDate &d) { return log << d.toString(Qt::ISODate); }
inline NecroLog &operator<<(NecroLog log, const QDateTime &d) { return log << d.toString(Qt::ISODate); }
inline NecroLog &operator<<(NecroLog log, const QTime &d) {  return log << d.toString(Qt::ISODate); }
