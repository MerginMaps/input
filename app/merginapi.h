#ifndef MERGINAPI_H
#define MERGINAPI_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QEventLoop>
#include <memory>
#include <QFile>

enum ProjectStatus
{
  NoVersion,
  UpToDate,
  OutOfDate,
  Modified
};
Q_ENUMS( ProjectStatus )

struct MerginProject {
    QString name;
    QStringList tags;
    QDateTime created;
    QDateTime updated; // local version of project files
    QDateTime serverUpdated; // available version of project files on server
    QDateTime lastSync; // local datetime of download/upload/update project
    bool pending = false; // if there is a pending request for downlaod/update a project
    ProjectStatus status = NoVersion;
    int size;
    int filesCount;
};

struct MerginFile {
    QString path;
    QString checksum;
    qint64 size;
};


typedef QList<std::shared_ptr<MerginProject>> ProjectList;

class MerginApi: public QObject {
    Q_OBJECT
    Q_PROPERTY(QString username READ username NOTIFY authChanged)
     Q_PROPERTY(QString apiRoot READ apiRoot WRITE setApiRoot NOTIFY apiRootChanged)
public:
    explicit MerginApi(const QString& dataDir, QObject* parent = nullptr );
    ~MerginApi() = default;

    /**
     * Sends non-blocking GET request to the server to listProjects. On listProjectsReplyFinished,
     * when a response is received, parses project json, writes it to a cache text file and sets mMerginProjects.
     * Eventually emits listProjectsFinished on which ProjectPanel (qml component) updates content.
     * If listing has been successful, updates cached merginProjects list.
     * @param withFilter If true, applies "input" tag in request.
     */
    Q_INVOKABLE void listProjects(QString filterTag = QStringLiteral("input_use"));

    /**
     * Sends non-blocking GET request to the server to download a project with a given name. On downloadProjectReplyFinished,
     * when a response is received, parses data-stream and creates files. Eventually emits syncProjectFinished on which
     * MerginProjectModel updates status of the project item. On syncProjectFinished, ProjectModel adds the project item to the project list.
     * If download has been successful, updates cached merginProjects list.
     * Emits also notify signal with a message for the GUI.
     * @param projectName Name of project to download.
     */
    Q_INVOKABLE void downloadProject(QString projectName);

    /**
     * Sends non-blocking POST request to the server to update a project with a given name. On downloadProjectReplyFinished,
     * when a response is received, parses data-stream to files and rewrites local files with them. Extra files which don't match server
     * files are removed. Eventually emits syncProjectFinished on which MerginProjectModel updates status of the project item.
     * If update has been successful, updates cached merginProjects list.
     * Emits also notify signal with a message for the GUI.
     * @param projectName Name of project to update.
     */
    Q_INVOKABLE void updateProject(QString projectName);

    /**
     * Sends non-blocking POST request to the server to upload changes in a project with a given name.
     * Firstly updateProject is triggered to fetch new changes. If it was successful, sends update post request with list of local changes
     * and modified/newly added files in JSON. Eventually emits syncProjectFinished on which MerginProjectModel updates status of the project item.
     * Emits also notify signal with a message for the GUI.
     * @param projectName Name of project to upload.
     */
    Q_INVOKABLE void uploadProject(QString projectName);

     /**
     * Currently no auth service is used, only "username:password" is encoded and asign to mToken.
     * @param username
     * @param password
     */
    Q_INVOKABLE void authorize(QString username, QString password);
    Q_INVOKABLE void clearAuth();
    Q_INVOKABLE void resetApiRoot();
    Q_INVOKABLE bool hasAuthData();

    // Test functions
    void createProject(QString projectName);
    void deleteProject(QString projectName);

    ProjectList projects();

    QString username() const;

    QString apiRoot() const;
    void setApiRoot(const QString &apiRoot);

signals:
    void listProjectsFinished(ProjectList merginProjects);
    void syncProjectFinished(QString projectDir, QString projectName, bool successfully = true);
    void reloadProject(QString projectDir);
    void networkErrorOccurred(QString message, QString additionalInfo);
    void notify(QString message);
    void merginProjectsChanged();
    void authRequested();
    void authChanged();
    void authFailed();
    void apiRootChanged();
    void projectCreated(QString projectName);
    void serverProjectDeleted(QString projectName);

public slots:
    void projectDeleted(QString projectName);

private slots:
    void listProjectsReplyFinished();
    void downloadProjectReplyFinished(); // download + update
    void uploadProjectReplyFinished();
    void updateInfoReplyFinished();
    void uploadInfoReplyFinished();
    void cacheProjects();
    void continueWithUpload(QString projectDir, QString projectName, bool successfully = true);
    void setUpdateToProject(QString projectDir, QString projectName, bool successfully);
    void saveAuthData();
    void createProjectFinished();
    void deleteProjectFinished();
    void authorizeFinished();


private:
    ProjectList parseProjectsData(const QByteArray &data, bool dataFromServer = false);
    bool cacheProjectsData(const QByteArray &data);
    void handleDataStream(QNetworkReply* r, QString projectDir, bool overwrite);
    bool saveFile(const QByteArray &data, QFile &file, bool closeFile);
    void createPathIfNotExists(QString filePath);
    ProjectStatus getProjectStatus(QDateTime localUpdated, QDateTime updated, QDateTime lastSync, QDateTime lastMod);
    QByteArray getChecksum(QString filePath);
    QSet<QString> listFiles(QString projectPath);
    void downloadProjectFiles(QString projectName, QByteArray json);
    void uploadProjectFiles(QString projectName, QByteArray json, QList<MerginFile> files);
    QHash<QString, QList<MerginFile>> parseAndCompareProjectFiles(QNetworkReply *r, bool isForUpdate);
    ProjectList updateMerginProjectList(ProjectList serverProjects);
    void deleteObsoleteFiles(QString projectName);
    QByteArray generateToken();
    void loadAuthData();
    static QString defaultApiRoot() { return "https://public.cloudmergin.com/"; }

    QNetworkAccessManager mManager;
    QString mApiRoot;
    ProjectList mMerginProjects;
    QString mDataDir; // dir with all projects
    QString mCacheFile;
    QString mUsername;
    QString mPassword;
    QHash<QUrl, QString>mPendingRequests;
    QSet<QString> mWaitingForUpload;
    QHash<QString, QSet<QString>> mObsoleteFiles;
    QSet<QString> mIgnoreFiles = QSet<QString>() << "gpkg-shm" << "gpkg-wal" << "qgs~" << "qgz~";

    const int CHUNK_SIZE = 65536;
};

#endif // MERGINAPI_H
