#include "recording/AbstractVideoEncoder.h"


AbstractVideoEncoder::AbstractVideoEncoder(QObject *pParent)
    : QObject(pParent),
      mFramesPerSecond(10),
      mVideoSize(640, 480),
      mVideoBitsPerSecond(1700000) // 1.7 Mbps
{
    // NOOP
}

AbstractVideoEncoder::~AbstractVideoEncoder() {
    // NOOP
}


void AbstractVideoEncoder::newChapter(const QString &pLabel, long timestamp) {
    Q_UNUSED(pLabel);
    Q_UNUSED(timestamp);
}
