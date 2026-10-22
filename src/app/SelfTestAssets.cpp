#include "app/SelfTestAssets.h"

#include <QDataStream>
#include <QFile>
#include <QIODevice>

namespace SelfTestAssets
{
    QByteArray toneWav()
    {
        const int  sampleCount = 800;
        QByteArray body(sampleCount, '\0');
        for (int i = 0; i < sampleCount; ++i)
        {
            body[i] = static_cast<char>(128 + (i % 32) - 16);
        }

        QByteArray  wav;
        QDataStream out(&wav, QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);

        out.writeRawData("RIFF", 4);
        out << qint32(36 + body.size());
        out.writeRawData("WAVE", 4);
        out.writeRawData("fmt ", 4);
        out << qint32(16) << quint16(1) << quint16(1) << qint32(8000) << qint32(8000) << quint16(1) << quint16(8);
        out.writeRawData("data", 4);
        out << qint32(body.size());
        out.writeRawData(body.constData(), body.size());

        return wav;
    }

    bool writeBytes(const QString &path, const QByteArray &data)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
        {
            return false;
        }

        return file.write(data) == data.size();
    }
} // namespace SelfTestAssets
