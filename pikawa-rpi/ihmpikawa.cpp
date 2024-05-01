#include "ihmpikawa.h"
#include "ui_ihmpikawa.h"
#include "GestionMagasin.h"
#include "BaseDeDonnees.h"
#include "Utilisateur.h"
#include "Communication.h"
#include <QDebug>

/**
 * @file ihmpikawa.cpp
 *
 * @brief Définition de la classe IhmPikawa
 * @author MDOIOUHOMA Nakib
 * @version 0.2
 */

/**
 * @brief Constructeur de la classe IhmPikawa
 *
 * @fn IhmPikawa::IhmPikawa
 * @param parent L'adresse de l'objet parent, si nullptr IHMPikawa sera la
 * fenêtre principale de l'application
 */
IhmPikawa::IhmPikawa(QWidget* parent) :
    QMainWindow(parent), ui(new Ui::IhmPikawa), gestionMagasin(new GestionMagasin(this)),
    bdd(BaseDeDonnees::getInstance()), communicationBluetooth(new Communication(this)),
    minuteurPreparationCafe(new QTimer(this))
{
    qDebug() << Q_FUNC_INFO;
    ui->setupUi(this);

    initialiserRessourcesGUI();
    fixerRaccourcisClavier();

    chargerListeUtilisateurs();
    initialiserListeCapsules();
    initialiserStocksRangeeCapsules();
    initialiserCapsulesRestantes();

    gererEvenements();
    changerEcranAccueil();

    rechercherCafetiere();
}

IhmPikawa::~IhmPikawa()
{
    delete ui;
    BaseDeDonnees::detruireInstance();
    qDebug() << Q_FUNC_INFO;
}

void IhmPikawa::fermerApplication()
{
    close();
}

void IhmPikawa::afficherEcran(IhmPikawa::Ecran ecran)
{
    qDebug() << Q_FUNC_INFO << "ecran" << ecran;
    ui->ecrans->setCurrentIndex(ecran);
}

void IhmPikawa::afficherEcranSuivant()
{
    int ecranCourant = IhmPikawa::Ecran(ui->ecrans->currentIndex());
    int ecranSuivant = (ecranCourant + 1) % int(IhmPikawa::NbEcrans);
    afficherEcran(IhmPikawa::Ecran(ecranSuivant));
}

void IhmPikawa::afficherEcranPrecedent()
{
    int ecranCourant   = ui->ecrans->currentIndex();
    int ecranPrecedent = (ecranCourant - 1) % int(IhmPikawa::NbEcrans);
    if(ecranPrecedent == -1)
        ecranPrecedent = int(IhmPikawa::NbEcrans) - 1;
    afficherEcran(IhmPikawa::Ecran(ecranPrecedent));
}

void IhmPikawa::changerEcranAccueil()
{
    afficherEcran(IhmPikawa::Ecran::EcranAccueil);
}

void IhmPikawa::changerMagasinCapsules()
{
    afficherEcran(IhmPikawa::Ecran::EcranMagasinCapsules);
}

void IhmPikawa::changerPreparationCafe()
{
    initialiserBoutonsCapsules();
    afficherEcran(IhmPikawa::Ecran::EcranPreprationCafe);
}
void IhmPikawa::changerEcranEtatPreparation()
{
    afficherEcran(IhmPikawa::Ecran::EcranEtatPreparation);
}

void IhmPikawa::afficherCafetiereDetectee(QString nom, QString adresse)
{
    qDebug() << Q_FUNC_INFO << "nom" << nom << "adresse" << adresse;
    // @todo prévoir une signalisation graphique
    ui->labelEtatCafetiere->setText(QString("Cafetière ") + QString(nom) + QString(" détectée"));
}

void IhmPikawa::afficherCafetiereConnectee(QString nom, QString adresse)
{
    qDebug() << Q_FUNC_INFO << "nom" << nom << "adresse" << adresse;
    // @todo prévoir une signalisation graphique
    ui->labelEtatCafetiere->setText(QString("Cafetière ") + QString(nom) + QString(" connectée"));
}

void IhmPikawa::afficherCafetiereDeconnectee()
{
    qDebug() << Q_FUNC_INFO;
    // @todo prévoir une signalisation graphique
    ui->labelEtatCafetiere->setText(QString("Cafetière déconnectée"));
}

void IhmPikawa::demarrerCommunication(QString nom, QString adresse)
{
    Q_UNUSED(nom)
    Q_UNUSED(adresse)

    if(!communicationBluetooth->estConnecte())
    {
        qDebug() << Q_FUNC_INFO;
        communicationBluetooth->desactiverLaDecouverte();
        communicationBluetooth->connecter();
    }
}

void IhmPikawa::demanderEtatMagasin(QString nom, QString adresse)
{
    Q_UNUSED(nom)
    Q_UNUSED(adresse)

    qDebug() << Q_FUNC_INFO;
    communicationBluetooth->envoyerTrame("#PIKAWA~M~");
}

void IhmPikawa::gererEtatMagasin(QStringList presenceCapsules)
{
    qDebug() << Q_FUNC_INFO << "presenceCapsules" << presenceCapsules;
    for(int i = 0; i < presenceCapsules.size(); i++)
    {
        if(presenceCapsules.at(i) == "1")
        {
            boutonsChoixCapsules.at(i)->setEnabled(true);
        }
        else
        {
            boutonsChoixCapsules.at(i)->setEnabled(false);
        }
    }
    qDebug() << "Mise à jour de l'état du magasin";
}

void IhmPikawa::gererEtatPreparation(int etat)
{
    qDebug() << Q_FUNC_INFO << "etat" << etat;

    changerEcranEtatPreparation();
    if(etat == EtatPreparation::Repos)
    {
        // Café prêt ou au repos
        afficherPreparationCafePret();
        decrementerNbCapsules();
    }
    else if(etat == EtatPreparation::EnCours)
    {
        afficherPreparationCafeEncours();
    }
    else if(etat == EtatPreparation::PreparationImpossible)
    {
        // Impossible (bac plein, ...)
        afficherPreparationImpossible();
    }
    else if(etat == EtatPreparation::ErreurCapsule)
    {
        // Erreur capsule
        afficherErreurCapsule();
    }
    else
    {
        qDebug() << Q_FUNC_INFO << "Erreur : état inconnu !" << etat;
        changerEcranAccueil();
    }
}

void IhmPikawa::selectionnerCapsule()
{
    QPushButton* boutonChoixCapsule = qobject_cast<QPushButton*>(sender());
    int          rangee             = rechercherRangee(boutonChoixCapsule);

    qDebug() << Q_FUNC_INFO << "bouton" << boutonChoixCapsule->text();
    qDebug() << Q_FUNC_INFO << "checked" << boutonChoixCapsule->isChecked();
    qDebug() << Q_FUNC_INFO << "rangee" << rangee;

    // Bouton sélectionné ?
    if(boutonChoixCapsule->isChecked())
    {
        // déselectionner les autres
        deselectionnerRangee(boutonChoixCapsule);
    }

    // Aucune rangée sélectionnée ?
    if(rechercherRangeeSelectionnee() == 0)
    {
        ui->boutonCafeCourt->setEnabled(false);
        ui->boutonCafeLong->setEnabled(false);
    }
    else
    {
        ui->boutonCafeCourt->setEnabled(true);
        ui->boutonCafeLong->setEnabled(true);
    }
}

void IhmPikawa::preparerCafeCourt()
{
    qDebug() << Q_FUNC_INFO;
    int rangeeSelectionnee = rechercherRangeeSelectionnee();
    if(rangeeSelectionnee != 0 && communicationBluetooth->estConnecte())
    {
        QString trame = "#PIKAWA~P~" + QString::number(rangeeSelectionnee) + "~1~\r\n";
        communicationBluetooth->envoyerTrame(trame);
        ui->boutonCafeCourt->setChecked(false);
        QPushButton* boutonRangee = boutonsChoixCapsules[rangeeSelectionnee - 1]; // Trouver le bouton de la rangée sélectionnée
        deselectionnerRangee(boutonRangee);
    }
    else
    {
        qDebug() << "Impossible de préparer ce café court !";
    }
}

void IhmPikawa::preparerCafeLong()
{
    qDebug() << Q_FUNC_INFO;
    int rangeeSelectionnee = rechercherRangeeSelectionnee();
    if(rangeeSelectionnee != 0 && communicationBluetooth->estConnecte())
    {
        QString trame = "#PIKAWA~P~" + QString::number(rangeeSelectionnee) + "~3~\r\n";

        communicationBluetooth->envoyerTrame(trame);
        ui->boutonCafeLong->setChecked(false);
        QPushButton* boutonRangee = boutonsChoixCapsules[rangeeSelectionnee - 1];
        deselectionnerRangee(boutonRangee);
    }
    else
    {
        qDebug() << "Impossible de préparer ce café long !";
    }
}

void IhmPikawa::afficherPreparationCafeEncours()
{
    ui->progressionCafe->setVisible(true);
    ui->progressionCafe->setValue(0);
    ui->etatCafePreparation->setPixmap(
                QPixmap(QString::fromUtf8("../images/iconeCafetiereVierge.png")));
    minuteurPreparationCafe->start(1000); // 1000 ms = 1 seconde
}

void IhmPikawa::afficherPreparationCafePret()
{
    ui->etatCafePreparation->setPixmap(
                QPixmap(QString::fromUtf8("../images/iconeCafetiereFonctionnel.png")));
    ui->progressionCafe->setValue(100);
    QTimer::singleShot(2000, this, &IhmPikawa::changerEcranAccueil);
}

void IhmPikawa::mettreAJourBarreProgression()
{
    static int progression = 0;

    // Incrémentez la progression de 20% toutes les secondes
    progression += 20;

    // Mettez à jour la valeur de la barre de progression
    ui->progressionCafe->setValue(progression);

    if(progression >= 100)
    {
        minuteurPreparationCafe->stop();
    }
}

void IhmPikawa::afficherErreurCapsule()
{
    ui->etatCafePreparation->setPixmap(
                QPixmap(QString::fromUtf8("../images/iconeCafetiereNonFonctionnel.png")));
    // @todo A remplacer par un bouton retour à l'accueil
    QTimer::singleShot(2000, this, &IhmPikawa::changerEcranAccueil);
}

void IhmPikawa::afficherPreparationImpossible()
{
    ui->etatCafePreparation->setPixmap(
                QPixmap(QString::fromUtf8("../images/iconeCafetiereNonFonctionnel.png")));
    // @todo A remplacer par un bouton retour à l'accueil
    QTimer::singleShot(2000, this, &IhmPikawa::changerEcranAccueil);
}

// Méthodes privées
void IhmPikawa::initialiserRessourcesGUI()
{
    // Initialisation des listes déroulantes capsules
    listesDeroulantesCapsules.push_back(ui->listeCapsulesR1);
    listesDeroulantesCapsules.push_back(ui->listeCapsulesR2);
    listesDeroulantesCapsules.push_back(ui->listeCapsulesR3);
    listesDeroulantesCapsules.push_back(ui->listeCapsulesR4);
    listesDeroulantesCapsules.push_back(ui->listeCapsulesR5);
    listesDeroulantesCapsules.push_back(ui->listeCapsulesR6);
    listesDeroulantesCapsules.push_back(ui->listeCapsulesR7);
    listesDeroulantesCapsules.push_back(ui->listeCapsulesR8);

    // Initialisation des stocks de rangees capsules
    stocksRangeesCapsules.push_back(ui->stockR1);
    stocksRangeesCapsules.push_back(ui->stockR2);
    stocksRangeesCapsules.push_back(ui->stockR3);
    stocksRangeesCapsules.push_back(ui->stockR4);
    stocksRangeesCapsules.push_back(ui->stockR5);
    stocksRangeesCapsules.push_back(ui->stockR6);
    stocksRangeesCapsules.push_back(ui->stockR7);
    stocksRangeesCapsules.push_back(ui->stockR8);

    // Initialisation des boutons de choix de capsules
    boutonsChoixCapsules.push_back(ui->boutonChoixCapsule1);
    boutonsChoixCapsules.push_back(ui->boutonChoixCapsule2);
    boutonsChoixCapsules.push_back(ui->boutonChoixCapsule3);
    boutonsChoixCapsules.push_back(ui->boutonChoixCapsule4);
    boutonsChoixCapsules.push_back(ui->boutonChoixCapsule5);
    boutonsChoixCapsules.push_back(ui->boutonChoixCapsule6);
    boutonsChoixCapsules.push_back(ui->boutonChoixCapsule7);
    boutonsChoixCapsules.push_back(ui->boutonChoixCapsule8);

    // Initialisation des LCDNumber capsules
    listeLCDNumberCapsules.push_back(ui->capsuleRestantesR1);
    listeLCDNumberCapsules.push_back(ui->capsuleRestantesR2);
    listeLCDNumberCapsules.push_back(ui->capsuleRestantesR3);
    listeLCDNumberCapsules.push_back(ui->capsuleRestantesR4);
    listeLCDNumberCapsules.push_back(ui->capsuleRestantesR5);
    listeLCDNumberCapsules.push_back(ui->capsuleRestantesR6);
    listeLCDNumberCapsules.push_back(ui->capsuleRestantesR7);
    listeLCDNumberCapsules.push_back(ui->capsuleRestantesR8);

    // Définition du texte du label de l'état de la cafetière
    ui->labelEtatCafetiere->setText(QString("Cafetière déconnectée"));

    // @todo gérer le café préféré
    ui->boutonCafePrefere->setEnabled(false);
}

void IhmPikawa::fixerRaccourcisClavier()
{
    QAction* quitter = new QAction(this);
    quitter->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    addAction(quitter);
    connect(quitter, SIGNAL(triggered()), this, SLOT(fermerApplication()));

    QAction* actionAllerDroite = new QAction(this);
    actionAllerDroite->setShortcut(QKeySequence(Qt::Key_Right));
    addAction(actionAllerDroite);
    connect(actionAllerDroite, SIGNAL(triggered()), this, SLOT(afficherEcranSuivant()));

    QAction* actionAllerGauche = new QAction(this);
    actionAllerGauche->setShortcut(QKeySequence(Qt::Key_Left));
    addAction(actionAllerGauche);
    connect(actionAllerGauche, SIGNAL(triggered()), this, SLOT(afficherEcranPrecedent()));
}

void IhmPikawa::gererEvenements()
{
    qDebug() << Q_FUNC_INFO;
    // Navigation dans l'IHM
    connect(ui->selectionMagasinCapsules,
            &QPushButton::clicked,
            this,
            &IhmPikawa::changerMagasinCapsules);
    connect(ui->retourAccueilDeCafe, &QPushButton::clicked, this, &IhmPikawa::changerEcranAccueil);
    connect(ui->selectionPreparationCafe,
            &QPushButton::clicked,
            this,
            &IhmPikawa::changerPreparationCafe);
    connect(ui->retourAccueilDeMachine,
            &QPushButton::clicked,
            this,
            &IhmPikawa::changerEcranAccueil);

    // Les boutons de sélection de capsules
    for(int i = 0; i < boutonsChoixCapsules.size(); ++i)
    {
        connect(boutonsChoixCapsules[i],
                &QPushButton::clicked,
                this,
                &IhmPikawa::selectionnerCapsule);
    }

    // Les boutons de préparation de café
    connect(ui->boutonCafeCourt, &QPushButton::clicked, this, &IhmPikawa::preparerCafeCourt);
    connect(ui->boutonCafeLong, &QPushButton::clicked, this, &IhmPikawa::preparerCafeLong);

    // signaux/slot de la communication
    connect(communicationBluetooth,
            &Communication::cafetiereDetectee,
            this,
            &IhmPikawa::demarrerCommunication);
    connect(communicationBluetooth,
            &Communication::cafetiereDetectee,
            this,
            &IhmPikawa::afficherCafetiereDetectee);
    connect(communicationBluetooth,
            &Communication::cafetiereConnectee,
            this,
            &IhmPikawa::demanderEtatMagasin);
    connect(communicationBluetooth,
            &Communication::cafetiereConnectee,
            this,
            &IhmPikawa::afficherCafetiereConnectee);
    connect(communicationBluetooth,
            &Communication::cafetiereDeconnectee,
            this,
            &IhmPikawa::afficherCafetiereDeconnectee);
    connect(communicationBluetooth,
            &Communication::etatMagasin,
            this,
            &IhmPikawa::gererEtatMagasin);
    connect(communicationBluetooth,
            &Communication::cafeEnPreparation,
            this,
            &IhmPikawa::gererEtatPreparation);

    connect(minuteurPreparationCafe,
            &QTimer::timeout,
            this,
            &IhmPikawa::mettreAJourBarreProgression);
}

void IhmPikawa::initialiserListeCapsules()
{
    QVector<QStringList> listeCapsules = gestionMagasin->getListeCapsules();
    for(int i = 0; i < listesDeroulantesCapsules.size(); ++i)
    {
        listesDeroulantesCapsules[i]->clear();
        for(int j = 0; j < listeCapsules.size(); ++j)
        {
            QFont formatFont = listesDeroulantesCapsules[i]->font();
            formatFont.setCapitalization(QFont::Capitalize);
            listesDeroulantesCapsules[i]->setFont(formatFont);
            listesDeroulantesCapsules[i]->addItem(
                        listeCapsules[j].at(GestionMagasin::TableCapsule::DESIGNATION));
        }
        listesDeroulantesCapsules[i]->addItem("Vide");
        listesDeroulantesCapsules[i]->addItem("Aucune");
        listesDeroulantesCapsules[i]->setCurrentIndex(listesDeroulantesCapsules[i]->count() - 1);
    }
}

void IhmPikawa::initialiserStocksRangeeCapsules()
{
    for(int i = 0; i < stocksRangeesCapsules.size(); ++i)
    {
        // pour l'instant, par défaut 0
        stocksRangeesCapsules[i]->setValue(0);
    }
}

void IhmPikawa::initialiserBoutonsCapsules()
{
    qDebug() << Q_FUNC_INFO;
    for(int i = 0; i < listesDeroulantesCapsules.size(); ++i)
    {
        if(listesDeroulantesCapsules[i]->currentText() != "Vide" &&
                listesDeroulantesCapsules[i]->currentText() != "Aucune" &&
                stocksRangeesCapsules[i]->value() > 0)
        {
            QFont formatFont = boutonsChoixCapsules[i]->font();
            formatFont.setCapitalization(QFont::Capitalize);
            boutonsChoixCapsules[i]->setFont(formatFont);
            boutonsChoixCapsules[i]->setText(listesDeroulantesCapsules[i]->currentText());
            boutonsChoixCapsules[i]->setEnabled(true);
        }
        else
        {
            boutonsChoixCapsules[i]->setText("");
            boutonsChoixCapsules[i]->setEnabled(false);
        }
    }
}

void IhmPikawa::chargerListeUtilisateurs()
{
    QVector<QStringList> listeUtilisateursBDD;
    QString              requeteSQL = "SELECT * FROM Utilisateur";
    bdd->recuperer(requeteSQL, listeUtilisateursBDD);
    qDebug() << Q_FUNC_INFO << "listeUtilisateurs" << listeUtilisateursBDD;
    for(int i = 0; i < listeUtilisateursBDD.size(); ++i)
    {
        listeUtilisateurs.push_back(new Utilisateur(listeUtilisateursBDD.at(i)));
    }
}

void IhmPikawa::rechercherCafetiere()
{
    communicationBluetooth->activerLaDecouverte();
}

void IhmPikawa::initialiserCapsulesRestantes()
{
    for(int i = 0; i < listeLCDNumberCapsules.size(); ++i)
    {
        listeLCDNumberCapsules[i]->display(0);
    }
}

int IhmPikawa::rechercherRangee(QPushButton* bouton)
{
    if(bouton == nullptr)
        return 0;

    for(int i = 0; i < boutonsChoixCapsules.size(); ++i)
    {
        if(bouton == boutonsChoixCapsules[i])
            return i + 1;
    }
    return 0;
}

int IhmPikawa::rechercherRangeeSelectionnee()
{
    for(int i = 0; i < boutonsChoixCapsules.size(); ++i)
    {
        if(boutonsChoixCapsules[i]->isChecked())
            return i + 1; // de 1 à 8
    }
    return 0;
}

void IhmPikawa::deselectionnerRangee(QPushButton* bouton)
{
    if(bouton == nullptr)
        return;

    for(int i = 0; i < boutonsChoixCapsules.size(); ++i)
    {
        if(bouton != boutonsChoixCapsules[i])
            boutonsChoixCapsules[i]->setChecked(false);
    }
}
void IhmPikawa::decrementerNbCapsules()
{
    int rangeeSelectionnee = rechercherRangeeSelectionnee();
    if (rangeeSelectionnee >= 1 && rangeeSelectionnee <= listeLCDNumberCapsules.size()) // Vérifier que la rangée est valide
    {
        int capsulesRestantes = listeLCDNumberCapsules[rangeeSelectionnee - 1]->value(); // Récupérer le nombre de capsules restantes
        if (capsulesRestantes > 0)
        {
            capsulesRestantes--; // Décrémenter le nombre de capsules

            listeLCDNumberCapsules[rangeeSelectionnee - 1]->display(capsulesRestantes); // Mettre à jour l'affichage du nombre de capsules
        }
    }
}
