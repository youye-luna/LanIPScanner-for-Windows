#pragma once

#include <QString>

/// 摄像头识别结果
struct CameraDetection
{
    bool isCamera = false;
    /// 命中的证据摘要（如 "RTSP;HTTP-强指纹"），未命中为空
    QString evidence;
};

/// 网络摄像头（IPC / DVR / NVR 等视频监控设备）识别。
/// 综合 RTSP 应答、HTTP 指纹、厂商私有端口、MAC 厂商前缀与主机名加权打分，
/// 累计分数达到阈值才判定为摄像头，尽量降低误报。
namespace CameraDetector
{
CameraDetection detect(const QString &ip, const QString &macAddress, const QString &hostName);
} // namespace CameraDetector
