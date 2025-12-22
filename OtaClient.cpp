// // ota_manager.cpp
// #include "OtaClient.h"
// #include <curl/curl.h>
// #include <openssl/sha.h>
// #include <json/json.h>
// #include <fstream>
// #include <iostream>
// #include <sstream>
// #include <sys/stat.h>
// #include <unistd.h>
// #include <filesystem>
//
// namespace fs = std::filesystem;
//
// // HTTP写数据回调
// static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
//     ((std::string*)userp)->append((char*)contents, size * nmemb);
//     return size * nmemb;
// }
//
// // HTTP下载写文件回调
// static size_t writeFileCallback(void* ptr, size_t size, size_t nmemb, void* stream) {
//     std::ofstream* out = static_cast<std::ofstream*>(stream);
//     out->write(static_cast<char*>(ptr), size * nmemb);
//     return size * nmemb;
// }
//
// // 进度回调
// static int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
//                            curl_off_t ultotal, curl_off_t ulnow) {
//     if (auto* cb = static_cast<OtaManager::ProgressCallback*>(clientp)) {
//         if (dltotal > 0) {
//             int progress = static_cast<int>((dlnow * 100) / dltotal);
//             (*cb)("download", progress);
//         }
//     }
//     return 0;
// }
//
// OtaManager::OtaManager(const std::string& deviceId,
//                      const std::string& serverUrl,
//                      const std::string& workDir)
//     : deviceId_(deviceId), serverUrl_(serverUrl), workDir_(workDir) {
//     curl_global_init(CURL_GLOBAL_DEFAULT);
// }
//
// OtaManager::~OtaManager() {
//     curl_global_cleanup();
// }
//
// bool OtaManager::initialize() {
//     // 创建工作目录
//     if (!fs::exists(workDir_)) {
//         if (!fs::create_directories(workDir_)) {
//             return false;
//         }
//     }
//     return true;
// }
//
// bool OtaManager::checkForUpdates() {
//     std::string url = serverUrl_ + "/api/updates?device=" + deviceId_;
//     std::string response;
//
//     if (!httpGet(url, response)) {
//         return false;
//     }
//
//     // 解析JSON响应
//     Json::Value root;
//     Json::CharReaderBuilder reader;
//     std::stringstream ss(response);
//     std::string errors;
//
//     if (!Json::parseFromStream(reader, ss, &root, &errors)) {
//         return false;
//     }
//
//     std::lock_guard<std::mutex> lock(mutex_);
//     updateList_.clear();
//
//     if (root.isMember("updates") && root["updates"].isArray()) {
//         for (const auto& item : root["updates"]) {
//             UpdateInfo info;
//             info.version = item.get("version", "").asString();
//             info.filePath = item.get("filePath", "").asString();
//             info.sha256 = item.get("sha256", "").asString();
//             info.fileSize = item.get("fileSize", 0).asUInt64();
//             info.description = item.get("description", "").asString();
//
//             std::string typeStr = item.get("type", "executable").asString();
//             if (typeStr == "firmware") info.type = UpdateType::FIRMWARE;
//             else if (typeStr == "library") info.type = UpdateType::LIBRARY;
//             else if (typeStr == "config") info.type = UpdateType::CONFIG;
//             else if (typeStr == "resource") info.type = UpdateType::RESOURCE;
//             else info.type = UpdateType::EXECUTABLE;
//
//             updateList_.push_back(info);
//         }
//     }
//
//     return !updateList_.empty();
// }
//
// bool OtaManager::httpGet(const std::string& url, std::string& response) {
//     CURL* curl = curl_easy_init();
//     if (!curl) return false;
//
//     curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
//     curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
//     curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
//     curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
//
//     // 设置认证信息
//     if (!username_.empty() && !password_.empty()) {
//         curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
//         curl_easy_setopt(curl, CURLOPT_USERNAME, username_.c_str());
//         curl_easy_setopt(curl, CURLOPT_PASSWORD, password_.c_str());
//     }
//
//     CURLcode res = curl_easy_perform(curl);
//     long http_code = 0;
//     curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
//     curl_easy_cleanup(curl);
//
//     return (res == CURLE_OK) && (http_code == 200);
// }
//
// bool OtaManager::httpDownload(const std::string& url, const std::string& localPath,
//                             ProgressCallback progressCb) {
//     CURL* curl = curl_easy_init();
//     if (!curl) return false;
//
//     std::ofstream outFile(localPath, std::ios::binary);
//     if (!outFile) return false;
//
//     curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
//     curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFileCallback);
//     curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outFile);
//     curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L); // 5分钟超时
//     curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024L); // 1KB/s最低速度
//     curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);   // 60秒
//
//     // 设置进度回调
//     if (progressCb) {
//         curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
//         curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
//         curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progressCb);
//     }
//
//     // 设置认证信息
//     if (!username_.empty() && !password_.empty()) {
//         curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
//         curl_easy_setopt(curl, CURLOPT_USERNAME, username_.c_str());
//         curl_easy_setopt(curl, CURLOPT_PASSWORD, password_.c_str());
//     }
//
//     CURLcode res = curl_easy_perform(curl);
//     outFile.close();
//     curl_easy_cleanup(curl);
//
//     return res == CURLE_OK;
// }
//
// bool OtaManager::downloadUpdates(ProgressCallback progressCb) {
//     std::lock_guard<std::mutex> lock(mutex_);
//
//     for (size_t i = 0; i < updateList_.size(); ++i) {
//         const auto& update = updateList_[i];
//
//         if (progressCb) {
//             progressCb("准备下载: " + update.filePath, (i * 100) / updateList_.size());
//         }
//
//         std::string downloadUrl = serverUrl_ + "/download/" + update.version + "/" + update.filePath;
//         std::string localPath = workDir_ + "/" + update.filePath;
//
//         // 创建目录
//         fs::path path(localPath);
//         if (!fs::exists(path.parent_path())) {
//             fs::create_directories(path.parent_path());
//         }
//
//         auto fileProgressCb = [&, i](const std::string&, int progress) {
//             if (progressCb) {
//                 int overallProgress = (i * 100 + progress) / updateList_.size();
//                 progressCb("下载: " + update.filePath, overallProgress);
//             }
//         };
//
//         if (!httpDownload(downloadUrl, localPath, fileProgressCb)) {
//             return false;
//         }
//
//         // 验证文件大小
//         std::error_code ec;
//         auto fileSize = fs::file_size(localPath, ec);
//         if (ec || fileSize != update.fileSize) {
//             fs::remove(localPath);
//             return false;
//         }
//     }
//
//     return true;
// }
//
// bool OtaManager::calculateFileHash(const std::string& filePath, std::string& hash) {
//     std::ifstream file(filePath, std::ios::binary);
//     if (!file) return false;
//
//     SHA256_CTX sha256;
//     SHA256_Init(&sha256);
//
//     char buffer[4096];
//     while (file.read(buffer, sizeof(buffer))) {
//         SHA256_Update(&sha256, buffer, file.gcount());
//     }
//     SHA256_Update(&sha256, buffer, file.gcount());
//
//     unsigned char digest[SHA256_DIGEST_LENGTH];
//     SHA256_Final(digest, &sha256);
//
//     char hashStr[65];
//     for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
//         sprintf(hashStr + (i * 2), "%02x", digest[i]);
//     }
//     hashStr[64] = 0;
//     hash = hashStr;
//
//     return true;
// }
//
// bool OtaManager::verifyUpdates() {
//     std::lock_guard<std::mutex> lock(mutex_);
//
//     for (const auto& update : updateList_) {
//         std::string localPath = workDir_ + "/" + update.filePath;
//
//         if (!fs::exists(localPath)) {
//             return false;
//         }
//
//         std::string actualHash;
//         if (!calculateFileHash(localPath, actualHash)) {
//             return false;
//         }
//
//         if (actualHash != update.sha256) {
//             return false;
//         }
//     }
//
//     return true;
// }
//
// bool OtaManager::backupFile(const std::string& filePath) {
//     if (!fs::exists(filePath)) {
//         return true; // 文件不存在，不需要备份
//     }
//
//     std::string backupPath = workDir_ + "/backup/" + fs::path(filePath).filename().string();
//     fs::create_directories(fs::path(backupPath).parent_path());
//
//     std::error_code ec;
//     fs::copy_file(filePath, backupPath, fs::copy_options::overwrite_existing, ec);
//
//     if (!ec) {
//         backupFiles_[filePath] = backupPath;
//     }
//
//     return !ec;
// }
//
// bool OtaManager::restoreBackup(const std::string& filePath) {
//     auto it = backupFiles_.find(filePath);
//     if (it == backupFiles_.end()) {
//         return false;
//     }
//
//     std::error_code ec;
//     fs::copy_file(it->second, filePath, fs::copy_options::overwrite_existing, ec);
//     return !ec;
// }
//
// bool OtaManager::performUpdate(StatusCallback statusCb) {
//     std::lock_guard<std::mutex> lock(mutex_);
//
//     // 1. 备份原有文件
//     if (statusCb) statusCb("开始备份文件...", false);
//
//     for (const auto& update : updateList_) {
//         if (!backupFile(update.filePath)) {
//             if (statusCb) statusCb("备份文件失败: " + update.filePath, true);
//             return false;
//         }
//     }
//
//     // 2. 复制新文件
//     if (statusCb) statusCb("开始更新文件...", false);
//
//     for (size_t i = 0; i < updateList_.size(); ++i) {
//         const auto& update = updateList_[i];
//         std::string sourcePath = workDir_ + "/" + update.filePath;
//         std::string targetPath = update.filePath;
//
//         if (statusCb) {
//             statusCb("更新文件: " + update.filePath + " (" + std::to_string(i+1) +
//                     "/" + std::to_string(updateList_.size()) + ")", false);
//         }
//
//         // 确保目标目录存在
//         fs::create_directories(fs::path(targetPath).parent_path());
//
//         // 复制文件
//         std::error_code ec;
//         fs::copy_file(sourcePath, targetPath, fs::copy_options::overwrite_existing, ec);
//         if (ec) {
//             if (statusCb) statusCb("文件复制失败: " + targetPath, true);
//             rollback(statusCb);
//             return false;
//         }
//
//         // 设置可执行权限（如果是可执行文件或库）
//         if (update.type == UpdateType::EXECUTABLE || update.type == UpdateType::LIBRARY) {
//             makeExecutable(targetPath);
//         }
//     }
//
//     // 3. 重新加载服务（根据文件类型）
//     if (statusCb) statusCb("重新加载服务...", false);
//
//     for (const auto& update : updateList_) {
//         switch (update.type) {
//             case UpdateType::LIBRARY:
//                 reloadLibrary(update.filePath);
//                 break;
//             case UpdateType::EXECUTABLE:
//                 // 可能需要重启相关服务
//                 break;
//             default:
//                 break;
//         }
//     }
//
//     if (statusCb) statusCb("更新完成", false);
//     return true;
// }
//
// bool OtaManager::rollback(StatusCallback statusCb) {
//     if (statusCb) statusCb("开始回滚...", false);
//
//     bool success = true;
//     for (const auto& pair : backupFiles_) {
//         if (!restoreBackup(pair.first)) {
//             success = false;
//             if (statusCb) statusCb("回滚文件失败: " + pair.first, true);
//         }
//     }
//
//     if (statusCb) statusCb(success ? "回滚完成" : "回滚部分失败", !success);
//     return success;
// }
//
// bool OtaManager::reloadLibrary(const std::string& libraryPath) {
//     // 在实际实现中，这里可能需要调用dlclose/dlopen或者重启相关进程
//     // 这里返回true表示假设成功
//     return true;
// }
//
// bool OtaManager::makeExecutable(const std::string& filePath) {
//     return chmod(filePath.c_str(), 0755) == 0;
// }
//
// void OtaManager::setCredentials(const std::string& username, const std::string& password) {
//     username_ = username;
//     password_ = password;
// }
//
// std::vector<OtaManager::UpdateInfo> OtaManager::getUpdateList() const {
//     std::lock_guard<std::mutex> lock(mutex_);
//     return updateList_;
// }
