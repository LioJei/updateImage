// // ota_manager.h
// #pragma once
// #include <string>
// #include <vector>
// #include <functional>
// #include <map>
// #include <mutex>
//
// class OtaManager {
// public:
//     // 回调函数类型定义
//     using ProgressCallback = std::function<void(const std::string& task, int progress)>;
//     using StatusCallback = std::function<void(const std::string& status, bool isError)>;
//     using FileListCallback = std::function<void(const std::vector<std::string>& files)>;
//
//     // 更新类型
//     enum class UpdateType {
//         FIRMWARE,      // 固件更新
//         EXECUTABLE,    // 可执行文件
//         LIBRARY,       // 动态库
//         CONFIG,        // 配置文件
//         RESOURCE       // 资源文件
//     };
//
//     struct UpdateInfo {
//         std::string version;
//         std::string filePath;
//         std::string sha256;
//         uint64_t fileSize;
//         UpdateType type;
//         std::string description;
//     };
//
//     OtaManager(const std::string& deviceId,
//                const std::string& serverUrl,
//                const std::string& workDir = "/tmp/ota");
//     ~OtaManager();
//
//     // 初始化OTA管理器
//     bool initialize();
//
//     // 检查可用更新
//     bool checkForUpdates();
//
//     // 下载所有待更新文件
//     bool downloadUpdates(ProgressCallback progressCb = nullptr);
//
//     // 验证文件完整性
//     bool verifyUpdates();
//
//     // 执行更新操作
//     bool performUpdate(StatusCallback statusCb = nullptr);
//
//     // 回滚到上一个版本
//     bool rollback(StatusCallback statusCb);
//
//     // 获取更新信息
//     std::vector<UpdateInfo> getUpdateList() const;
//
//     // 设置认证信息
//     void setCredentials(const std::string& username, const std::string& password);
//
// private:
//     // HTTP相关方法
//     bool httpGet(const std::string& url, std::string& response);
//     bool httpDownload(const std::string& url, const std::string& localPath,
//                      ProgressCallback progressCb = nullptr);
//
//     // 文件操作
//     bool calculateFileHash(const std::string& filePath, std::string& hash);
//     bool backupFile(const std::string& filePath);
//     bool restoreBackup(const std::string& filePath);
//
//     // 平台相关方法
//     bool reloadLibrary(const std::string& libraryPath);
//     bool restartService(const std::string& serviceName);
//     bool makeExecutable(const std::string& filePath);
//
//     std::string deviceId_;
//     std::string serverUrl_;
//     std::string workDir_;
//     std::string username_;
//     std::string password_;
//
//     std::vector<UpdateInfo> updateList_;
//     std::map<std::string, std::string> backupFiles_;
//
//     mutable std::mutex mutex_;
// };
