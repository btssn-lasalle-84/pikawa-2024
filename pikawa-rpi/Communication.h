#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <QObject>
#include <QtBluetooth>

#define PREFIXE_NOM_CAFETIERE "pikawa"

class Communication : public QObject
{
    Q_OBJECT
  private:
    QBluetoothLocalDevice           interfaceLocale;
    QBluetoothDeviceDiscoveryAgent* agentDecouvreur;
    QBluetoothDeviceInfo            pikawa;
    bool                            pikawaDetecte;
    QBluetoothSocket*               socketBluetoothPikawa;
    QString                         trame;

    bool estBluetoothDisponible() const;
    void activerBluetooth();

  public:
    Communication(QObject* parent = nullptr);
    ~Communication();

    bool estConnecte() const;
    bool estDetecte() const;

  public slots:
    void activerLaDecouverte();
    void desactiverLaDecouverte();
    void connecter();
    void deconnecter();
    void connecterSocket();
    void deconnecterSocket();
    void lireDonneesDisponnible();
    void envoyerTrame(QString trame);

  signals:
    void cafetiereDetectee(QString nom, QString adresse);
    void rechercheTerminee(bool);
    void cafetiereConnectee(QString nom, QString adresse);
    void cafetiereDeconnectee();
    void etatMagasin(QStringList presenceCapsules); // R1 à R8
    void cafeEnPreparation(int code);
};

#endif // COMMUNICATION_H
