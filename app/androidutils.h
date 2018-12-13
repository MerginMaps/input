#ifndef ANDROIDUTILS_H
#define ANDROIDUTILS_H

#include <QObject>

class AndroidUtils: public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isAndroid READ isAndroid )
public:
    explicit AndroidUtils( QObject* parent = nullptr );

    bool isAndroid() const;

public slots:
    void showToast( QString message );

private:
    bool mIsAndroid;

};

#endif // ANDROIDUTILS_H
