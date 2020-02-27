#include <QCoreApplication>
#include <QDebug>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QThread>
#include <connection_thread.h>
#include <registry.h>
#include <dpms.h>
#include <output.h>
#include <iostream>
#include <QString>

using namespace KWayland::Client;

const QString VERSION = "1.0.1";

static bool beConnect = false;

static DpmsManager *pDpmsManager = nullptr;
static Dpms *pDpms = nullptr;
static Dpms::Mode mode = Dpms::Mode::Off;

void showDpms(const Dpms::Mode& mode )
{
    if (mode == Dpms::Mode::On)
    {
        std::cout << "DPMS mode: on" << std::endl;
    }
    else if (mode == Dpms::Mode::Off)
    {
        std::cout << "DPMS mode: Off" << std::endl;
    }
    else if (mode == Dpms::Mode::Standby)
    {
        std::cout << "DPMS mode: Standby" << std::endl;
    }
    else if (mode == Dpms::Mode::Suspend)
    {
        std::cout << "DPMS mode: Suspend" << std::endl;
    }
    else
    {
        std::cout << "DPMS mode: unknow" << std::endl;
    }

}

void getDpms(Registry* reg)
{
    QObject::connect(reg, &Registry::dpmsAnnounced,  [reg](quint32 name, quint32 version){
         qDebug() << "dpmsAnnounced with name :" << name << " & version :" << version;
         qDebug() << "reg pt :" << reg << "\treg is valid :" << reg->isValid();
         pDpmsManager = reg->createDpmsManager(name, version);
         if (!pDpmsManager)
         {
             qDebug() << "DpmsManager is nullptr!";
             return;

         }
         if (!pDpmsManager->isValid())
         {
             qDebug() << "DpmsManager is not valid!";
             return;
         }

         QObject::connect(reg, &Registry::outputAnnounced, [reg](quint32 name, quint32 version) {

             beConnect = true;
             auto output = reg->createOutput(name, version);
             if (!output || !output->isValid()) {
                 qDebug() << "get output is null or not valid!";
                 return;
             }

             //Please note that all properties of Output are not valid until the changed signal has been emitted.
            //The wayland server is pushing the information in an async way to the Output instance.
            //By emitting changed the Output indicates that all relevant information is available.
             QObject::connect(output, &Output::changed, [output] {
                 beConnect = true;
                 //qDebug() << "output h:" << output->pixelSize().height();
                 //qDebug() << "output w:" << output->pixelSize().width();
                 pDpms = pDpmsManager->getDpms(output);
                 if (!pDpms)
                 {
                     qDebug() << "pDpms is nullptr!";
                     return;
                 }
                 if (!pDpms->isValid())
                 {
                     qDebug() << "dpms is not valid!";
                     return;
                 }

                 if (pDpms->mode() == Dpms::Mode::On)
                 {
                     showDpms(Dpms::Mode::On);
                 }

                 if (!pDpms->isSupported())
                 {
                     QObject::connect(pDpms, &Dpms::supportedChanged, [&] {
                         beConnect = true;
                         QObject::connect(pDpms, &Dpms::modeChanged, [&] {
                             beConnect = true;
                             qDebug() << "set dpms sucess";
                             showDpms(pDpms->mode());
                         });
                     });
                 }
                 else
                 {
                     QObject::connect(pDpms, &Dpms::modeChanged, [&] {
                         beConnect = true;
                         qDebug() << "set dpms sucess";
                         showDpms(pDpms->mode());
                     });
                 }
             });
         });
    });
}

void setDpms(Registry *reg)
{
    QObject::connect(reg, &Registry::dpmsAnnounced, [reg](quint32 name, quint32 version) {
        //qDebug() << "dpmsAnnounced with name :" << name << " & version :" << version;
        //qDebug() << "reg pt :" << reg << "\treg is valid :" << reg->isValid();
        pDpmsManager = reg->createDpmsManager(name, version);
        if (!pDpmsManager)
        {
            qDebug() << "DpmsManager is nullptr!";
            return;

        }
        if (!pDpmsManager->isValid())
        {
            qDebug() << "DpmsManager is not valid!";
            return;
        }

        QObject::connect(reg, &Registry::outputAnnounced, [reg](quint32 name, quint32 version) {

            beConnect = true;
            auto output = reg->createOutput(name, version);
            if (!output || !output->isValid()) {
                qDebug() << "get output is null or not valid!";
                return;
            }

            //Please note that all properties of Output are not valid until the changed signal has been emitted.
            //The wayland server is pushing the information in an async way to the Output instance.
            //By emitting changed the Output indicates that all relevant information is available.
            QObject::connect(output, &Output::changed, [output] {
                beConnect = true;
                //qDebug() << "output h:" << output->pixelSize().height();
                //qDebug() << "output w:" << output->pixelSize().width();
                pDpms = pDpmsManager->getDpms(output);
                if (!pDpms)
                {
                    qDebug() << "pDpms is nullptr!";
                    return;
                }
                if (!pDpms->isValid())
                {
                    qDebug() << "dpms is not valid!";
                    return;
                }

                if (!pDpms->isSupported())
                {
                    QObject::connect(pDpms, &Dpms::supportedChanged, [&] {
                        beConnect = true;
                        pDpms->requestMode(mode);
                        QObject::connect(pDpms, &Dpms::modeChanged, [&] {
                            beConnect = true;
                            qDebug() << "set dpms sucess";
                            showDpms(pDpms->mode());
                        });
                    });
                }
                else
                {
                    pDpms->requestMode(mode);
                    QObject::connect(pDpms, &Dpms::modeChanged, [&] {
                        beConnect = true;
                        qDebug() << "set dpms sucess";
                        showDpms(pDpms->mode());
                    });
                }
            });


        });
    });
}


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    auto conn = new ConnectionThread;
    auto thread = new QThread;
    Registry *reg = nullptr;

    // app name
    QCoreApplication::setApplicationName("dpms");
    // app version
    QCoreApplication::setApplicationVersion(VERSION); // app version


    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Set or get dpms information."));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption getOption(QStringList() << "g" << "get", QCoreApplication::translate("main", "Get dpms."));
    parser.addOption(getOption);
    QCommandLineOption setOption(QStringList() << "s" << "set",
                                 QCoreApplication::translate("main", "Set dpms to On/Standby/Suspend/Off"));
    setOption.setValueName("param");
    parser.addOption(setOption);

    parser.process(a);

    conn->moveToThread(thread);
    thread->start();

    conn->initConnection();

    QObject::connect(conn, &ConnectionThread::connected, [ & ] {

        beConnect = true;
        reg = new Registry;
        reg->create(conn);
        reg->setup();

        bool tmp = false;
        if (parser.isSet(getOption))
        {
            getDpms(reg);
        }
        else if (parser.isSet(setOption))
        {
            QString strSetOption = parser.value(setOption);
            if (strSetOption.compare(QString("On"), Qt::CaseInsensitive) == 0)
            {
                mode = Dpms::Mode::On;
                setDpms(reg);
            }
            else if (strSetOption.compare(QString("Off"), Qt::CaseInsensitive) == 0)
            {
                mode = Dpms::Mode::Off;
                setDpms(reg);
            }
            else if (strSetOption.compare(QString("Standby"), Qt::CaseInsensitive) == 0)
            {
                mode = Dpms::Mode::Standby;
                setDpms(reg);
            }
            else if (strSetOption.compare(QString("Suspend"), Qt::CaseInsensitive) == 0)
            {
                mode = Dpms::Mode::Suspend;
                setDpms(reg);
            }
            else
            {
                parser.showHelp();
            }
        }
        else
        {
            parser.showHelp();
        }
        int count = 100;
        do {
            beConnect = tmp;
            conn->roundtrip();
            QThread::msleep(100);
        } while (beConnect && count--);

        if (count <= 0)
        {
            qDebug() << "time out";
        }

        if (conn)
        {
            conn->deleteLater();
            thread->quit();
        }
        if (reg)
            reg->deleteLater();
        exit(0);
    });


   QObject::connect(conn, &ConnectionThread::failed, [conn] {
       qDebug() << "connect failed to wayland at socket:" << conn->socketName();
       beConnect = true;
   });

   QObject::connect(conn, &ConnectionThread::connectionDied, [ & ] {
       qDebug() << "connect failed to wayland at socket:" << conn->socketName();
       beConnect = true;
       if (reg)
           reg->deleteLater();
       if (pDpms)
           pDpms->destroy();

       exit(-1);
   });


    return a.exec();
}
