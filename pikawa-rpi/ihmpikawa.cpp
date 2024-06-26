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
 * @version 1.0
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
    minuteurPreparationCafe(new QTimer(this)), rangeeSelectionneePreparation(0),
    rangeeDernierCafe(0)
{
    qDebug() << Q_FUNC_INFO;
    ui->setupUi(this);

    initialiserRessourcesGUI();
    fixerRaccourcisClavier();

    chargerListeUtilisateurs();
    initialiserListeCapsules();
    initialiserStocksRangeeCapsules();
    initialiserCapsulesRestantes();

    afficherCafetiereDeconnectee();
    gererEvenements();
    changerEcranAccueil();

    rechercherCafetiere();

#ifdef PLEIN_ECRAN_PI
    showFullScreen();
#endif
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
    QPixmap iconeDetectee(QString::fromUtf8("../images/iconeCafetiereDetectee.png"));
    ui->etatCafetiere->setPixmap(iconeDetectee);
    ui->etatCafetiere->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->labelEtatCafetiere->setText(QString("Cafetière ") + QString(nom) + QString(" détectée"));
}

void IhmPikawa::afficherCafetiereConnectee(QString nom, QString adresse)
{
    qDebug() << Q_FUNC_INFO << "nom" << nom << "adresse" << adresse;
    QPixmap iconeConnectee(QString::fromUtf8("../images/iconeCafetiereFonctionnel.png"));
    ui->etatCafetiere->setPixmap(iconeConnectee);
    ui->etatCafetiere->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->labelEtatCafetiere->setText(QString("Cafetière ") + QString(nom) + QString(" connectée"));
}

void IhmPikawa::afficherCafetiereDeconnectee()
{
    qDebug() << Q_FUNC_INFO;
    ui->labelEtatCafetiere->setText(QString("Cafetière déconnectée"));
    QPixmap iconeDeconnectee(QString::fromUtf8("../images/iconeCafetiereNonFonctionnel.png"));
    ui->etatCafetiere->setPixmap(iconeDeconnectee);
    ui->etatCafetiere->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
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
    presenceCapsulesPikawa = presenceCapsules;
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
        afficherErreurCapsuleAbscente();
    }
    else if(etat == EtatPreparation::BacCapsulePlein)
    {
        // Erreur reservoir d'eau vide
        afficherErreurBacCapsulePlein();
    }
    else if(etat == EtatPreparation::ReservoirEauPlein)
    {
        // Erreur reservoir d'eau vide
        afficherErreurReservoirEauVide();
    }
    else if(etat == EtatPreparation::TasseAbscente)
    {
        // Erreur tasse abscente
        afficherErreurTasseAbscente();
    }
    else if(etat == EtatPreparation::CapsuleAbscente)
    {
        // Erreur capsule abscente
        afficherErreurBacCapsulePlein();
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

    qDebug() << Q_FUNC_INFO << "bouton" << boutonChoixCapsule->text() << "rangee" << rangee
             << "checked" << boutonChoixCapsule->isChecked();

    // Bouton sélectionné ?
    if(boutonChoixCapsule->isChecked())
    {
        // déselectionner les autres
        deselectionnerAutresRangees(boutonChoixCapsule);
    }

    // Aucune rangée sélectionnée ?
    if(rechercherRangeeSelectionnee() == 0)
    {
        ui->boutonCafeCourt->setEnabled(false);
        ui->boutonCafeLong->setEnabled(false);
        ui->boutonDernierCafe->setEnabled(false);
    }
    else
    {
        ui->boutonCafeCourt->setEnabled(true);
        ui->boutonCafeLong->setEnabled(true);
        if(!typeDernierCafe.isEmpty())
            ui->boutonDernierCafe->setEnabled(true);
    }
}

void IhmPikawa::preparerCafeCourt()
{
    int rangeeSelectionnee = rechercherRangeeSelectionnee();
    if(rangeeSelectionnee != 0 && communicationBluetooth->estConnecte())
    {
        qDebug() << Q_FUNC_INFO << "rangeeSelectionnee" << rangeeSelectionnee;
        QString trame = QString(DEBUT_TRAME) + QString(TRAME_SEPARATEUR) +
                        QString(TRAME_PREPARATION_CAFE) + QString(TRAME_SEPARATEUR) +
                        QString::number(rangeeSelectionnee) + QString(TRAME_SEPARATEUR) +
                        QString(CAFE_RISTRETTO) + QString(TRAME_SEPARATEUR);
        communicationBluetooth->envoyerTrame(trame);
        ui->boutonCafeCourt->setChecked(false);
        rangeeSelectionneePreparation = rangeeSelectionnee;
        deselectionnerRangee(rangeeSelectionnee);
        typeDernierCafe   = CAFE_RISTRETTO;
        rangeeDernierCafe = rangeeSelectionnee;
    }
}

void IhmPikawa::preparerCafeLong()
{
    int rangeeSelectionnee = rechercherRangeeSelectionnee();
    if(rangeeSelectionnee != 0 && communicationBluetooth->estConnecte())
    {
        qDebug() << Q_FUNC_INFO << "rangeeSelectionnee" << rangeeSelectionnee;
        QString trame = QString(DEBUT_TRAME) + QString(TRAME_SEPARATEUR) +
                        QString(TRAME_PREPARATION_CAFE) + QString(TRAME_SEPARATEUR) +
                        QString::number(rangeeSelectionnee) + QString(TRAME_SEPARATEUR) +
                        QString(CAFE_LUNGO) + QString(TRAME_SEPARATEUR);
        communicationBluetooth->envoyerTrame(trame);
        ui->boutonCafeLong->setChecked(false);
        rangeeSelectionneePreparation = rangeeSelectionnee;
        deselectionnerRangee(rangeeSelectionnee);
        typeDernierCafe   = CAFE_LUNGO;
        rangeeDernierCafe = rangeeSelectionnee;
    }
}

void IhmPikawa::preparerDernierCafe()
{
    // @todo Vérifier que le type et la rangée du dernier café soient encore disponible au niveau
    // magasin
    if(rangeeDernierCafe != 0 && communicationBluetooth->estConnecte())
    {
        if(typeDernierCafe == CAFE_RISTRETTO || typeDernierCafe == CAFE_LUNGO)
        {
            qDebug() << Q_FUNC_INFO << "rangeeDernierCafe" << rangeeDernierCafe;
            QString trame = QString(DEBUT_TRAME) + QString(TRAME_SEPARATEUR) +
                            QString(TRAME_PREPARATION_CAFE) + QString(TRAME_SEPARATEUR) +
                            QString::number(rangeeDernierCafe) + QString(TRAME_SEPARATEUR) +
                            typeDernierCafe + QString(TRAME_SEPARATEUR);
            communicationBluetooth->envoyerTrame(trame);
            ui->boutonDernierCafe->setChecked(false);
            rangeeSelectionneePreparation = rangeeDernierCafe;
            deselectionnerRangee(rangeeDernierCafe);
        }
    }
}

void IhmPikawa::afficherPreparationCafeEncours()
{
    qDebug() << Q_FUNC_INFO;
    ui->progressionCafe->setVisible(true);
    ui->progressionCafe->setValue(0);
    ui->etatCafePreparation->setPixmap(
      QPixmap(QString::fromUtf8("../images/iconeCafetiereVierge.png")));
    ui->etatBacCapsule->setPixmap(
      QPixmap(QString::fromUtf8("../images/iconeCafetiereFonctionnel.png")));
    ui->etatTasse->setPixmap(QPixmap(QString::fromUtf8("../images/iconeCafetiereFonctionnel.png")));
    ui->etatCapsule->setPixmap(
      QPixmap(QString::fromUtf8("../images/iconeCafetiereFonctionnel.png")));
    minuteurPreparationCafe->start(DUREE_PROGRESSION); // en ms
}

void IhmPikawa::afficherPreparationCafePret()
{
    qDebug() << Q_FUNC_INFO;
    ui->etatCafePreparation->setPixmap(
      QPixmap(QString::fromUtf8("../images/iconeCafetiereFonctionnel.png")));
    QTimer::singleShot(DUREE_AFFICHAGE, this, &IhmPikawa::changerEcranAccueil);
}

void IhmPikawa::mettreAJourBarreProgressionPreparationCafe()
{
    // Variables statiques pour conserver l'état entre les appels
    static int progression            = 0;
    static int dureeTotalePreparation = 0;

    if(progression == 0)
    {
        if(typeDernierCafe == CAFE_RISTRETTO)
        {
            dureeTotalePreparation = DUREE_CAFE_COURT;
        }
        else if(typeDernierCafe == CAFE_LUNGO)
        {
            dureeTotalePreparation = DUREE_CAFE_LONG;
        }
    }

    // Calcule le pas de progression
    int pasProgression = (100 * DUREE_PROGRESSION) / dureeTotalePreparation;

    // Mettre à jour la progression
    progression += pasProgression;

    // Vérifier si la progression est terminée
    if(progression >= 100)
    {
        minuteurPreparationCafe->stop();
        ui->progressionCafe->setValue(100);
        progression = 0; // Réinitialiser la progression pour la prochaine utilisation
    }
    else
    {
        ui->progressionCafe->setValue(progression);
    }
}

void IhmPikawa::afficherErreurCapsule()
{
    ui->progressionCafe->setVisible(false);
    ui->etatCafePreparation->setPixmap(
      QPixmap(QString::fromUtf8("../images/iconeCafetiereNonFonctionnel.png")));
    // @todo A remplacer par un bouton retour à l'accueil
    QTimer::singleShot(DUREE_AFFICHAGE, this, &IhmPikawa::changerEcranAccueil);
}

void IhmPikawa::afficherPreparationImpossible()
{
    ui->progressionCafe->setVisible(false);
    ui->etatCafePreparation->setPixmap(
      QPixmap(QString::fromUtf8("../images/iconeCafetiereNonFonctionnel.png")));
    // @todo A remplacer par un bouton retour à l'accueil
    QTimer::singleShot(DUREE_AFFICHAGE, this, &IhmPikawa::changerEcranAccueil);
}

void IhmPikawa::afficherErreurBacCapsulePlein()
{
    ui->progressionCafe->setVisible(false);
    ui->etatCafePreparation->setPixmap(
      QPixmap(QString::fromUtf8("../images/iconeCafetiereNonFonctionnel.png")));
    ui->etatBacCapsule->setPixmap(
      QPixmap(QString::fromUtf8("../images/iconeCafetiereNonFonctionnel.png")));
    QTimer::singleShot(DUREE_AFFICHAGE, this, &IhmPikawa::changerEcranAccueil);
}

void IhmPikawa::afficherErreurReservoirEauVide()
{
    ui->progressionCafe->setVisible(false);
    ui->etatCafePreparation->setPixmap(
      QPixmap(QString::fromUtf8("../images/iconeCafetiereNonFonctionnel.png")));
    QTimer::singleShot(DUREE_AFFICHAGE, this, &IhmPikawa::changerEcranAccueil);
}

void IhmPikawa::afficherErreurTasseAbscente()
{
    ui->progressionCafe->setVisible(false);
    ui->etatCafePreparation->setPixmap(
      QPixmap(QString::fromUtf8("../images/iconeCafetiereNonFonctionnel.png")));
    ui->etatTasse->setPixmap(
      QPixmap(QString::fromUtf8("../images/iconeCafetiereNonFonctionnel.png")));
    QTimer::singleShot(DUREE_AFFICHAGE, this, &IhmPikawa::changerEcranAccueil);
}

void IhmPikawa::afficherErreurCapsuleAbscente()
{
    ui->progressionCafe->setVisible(false);
    ui->etatCafePreparation->setPixmap(
      QPixmap(QString::fromUtf8("../images/iconeCafetiereNonFonctionnel.png")));
    ui->etatCapsule->setPixmap(
      QPixmap(QString::fromUtf8("../images/iconeCafetiereNonFonctionnel.png")));
    QTimer::singleShot(DUREE_AFFICHAGE, this, &IhmPikawa::changerEcranAccueil);
}

void IhmPikawa::modifierStock(int nbCapsules)
{
    QSpinBox* modificationStock   = qobject_cast<QSpinBox*>(sender());
    int       rangee              = rechercherRangee(modificationStock);
    QString   capsuleSelectionnee = listesDeroulantesCapsules[rangee - 1]->currentText();
    qDebug() << Q_FUNC_INFO << "rangee" << rangee << "nbCapsules" << nbCapsules
             << "capsuleSelectionnee" << capsuleSelectionnee;

    if(capsuleSelectionnee != "aucune")
    {
        qDebug() << Q_FUNC_INFO << "nbCapsules" << nbCapsules << "rangee" << rangee;
        QString requeteSQL = "UPDATE StockMagasin SET quantite='" + QString::number(nbCapsules) +
                             "' WHERE rangee='" + QString::number(rangee) + "'";
        if(bdd->executer(requeteSQL))
        {
            listeLCDNumberCapsules[rangee - 1]->display(nbCapsules);
            stocksRangeesCapsules[rangee - 1]->setValue(
              listeLCDNumberCapsules[rangee - 1]->value());
            int capsulesRestantesTotal = calculerTotalCapsulesRestantes();
            ui->capsuleTotalRestantes->display(capsulesRestantesTotal);
            if(capsulesRestantesTotal < SEUIL_CAPSULES_RESTANTES)
                ui->capsuleTotalRestantes->setStyleSheet(
                  "background-color: transparent; color: red;");
            else
                ui->capsuleTotalRestantes->setStyleSheet(
                  "background-color: transparent;color: green;");
        }
    }
}

void IhmPikawa::choisirCapsuleStock(int indexCapsule)
{
    QComboBox* listeDeroulanteCapsules = qobject_cast<QComboBox*>(sender());
    int        rangee                  = rechercherRangee(listeDeroulanteCapsules);
    QString    capsuleSelectionnee     = listesDeroulantesCapsules[rangee - 1]->currentText();
    qDebug() << Q_FUNC_INFO << "rangee" << rangee << "indexCapsule" << indexCapsule
             << "capsuleSelectionnee" << capsuleSelectionnee;

    if(capsuleSelectionnee != "aucune")
    {
        qDebug() << Q_FUNC_INFO << "rangee" << rangee << "actuel idCapsule"
                 << gestionMagasin->getIdCapsuleRangee(rangee) << "nouvel idCapsule"
                 << gestionMagasin->getIdCapsuleListe(indexCapsule);

        QString requeteSQL = "UPDATE StockMagasin SET idCapsule='" +
                             gestionMagasin->getIdCapsuleListe(indexCapsule) + "' WHERE rangee='" +
                             QString::number(rangee) + "'";
        if(bdd->executer(requeteSQL))
        {
            stocksRangeesCapsules[rangee - 1]->setEnabled(true);
        }
    }
    else
    {
        stocksRangeesCapsules[rangee - 1]->setEnabled(false);
    }
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
    ui->capsuleRestantesR1->setFixedSize(ui->capsuleRestantesR1->width() * 1.5,
                                         ui->capsuleRestantesR1->height() *
                                           1.5); // agrandit sa taille
    listeLCDNumberCapsules.push_back(ui->capsuleRestantesR2);
    ui->capsuleRestantesR2->setFixedSize(ui->capsuleRestantesR2->width() * 1.5,
                                         ui->capsuleRestantesR2->height() *
                                           1.5); // agrandit sa taille
    listeLCDNumberCapsules.push_back(ui->capsuleRestantesR3);
    ui->capsuleRestantesR3->setFixedSize(ui->capsuleRestantesR3->width() * 1.5,
                                         ui->capsuleRestantesR3->height() *
                                           1.5); // agrandit sa taille
    listeLCDNumberCapsules.push_back(ui->capsuleRestantesR4);
    ui->capsuleRestantesR4->setFixedSize(ui->capsuleRestantesR4->width() * 1.5,
                                         ui->capsuleRestantesR4->height() *
                                           1.5); // agrandit sa taille
    listeLCDNumberCapsules.push_back(ui->capsuleRestantesR5);
    ui->capsuleRestantesR5->setFixedSize(ui->capsuleRestantesR5->width() * 1.5,
                                         ui->capsuleRestantesR5->height() *
                                           1.5); // agrandit sa taille
    listeLCDNumberCapsules.push_back(ui->capsuleRestantesR6);
    ui->capsuleRestantesR6->setFixedSize(ui->capsuleRestantesR6->width() * 1.5,
                                         ui->capsuleRestantesR6->height() *
                                           1.5); // agrandit sa taille
    listeLCDNumberCapsules.push_back(ui->capsuleRestantesR7);
    ui->capsuleRestantesR7->setFixedSize(ui->capsuleRestantesR7->width() * 1.5,
                                         ui->capsuleRestantesR7->height() *
                                           1.5); // agrandit sa taille
    listeLCDNumberCapsules.push_back(ui->capsuleRestantesR8);
    ui->capsuleRestantesR8->setFixedSize(ui->capsuleRestantesR8->width() * 1.5,
                                         ui->capsuleRestantesR8->height() *
                                           1.5); // agrandit sa taille

    ui->capsuleTotalRestantes->setFixedSize(ui->capsuleTotalRestantes->width() * 2,
                                            ui->capsuleTotalRestantes->height() *
                                              2); // agrandit sa taille

    // Définition du texte du label de l'état de la cafetière
    ui->labelEtatCafetiere->setText(QString("Cafetière déconnectée"));

    ui->horizontalLayoutErreurs->setAlignment(Qt::AlignCenter);
    ui->verticalLayoutErreur1->setAlignment(Qt::AlignCenter);
    ui->verticalLayoutErreur2->setAlignment(Qt::AlignCenter);
    ui->verticalLayoutErreur3->setAlignment(Qt::AlignCenter);
    ui->verticalLayoutErreur4->setAlignment(Qt::AlignCenter);
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

    // La gestion du choix de capsules pour le stock
    for(int i = 0; i < listesDeroulantesCapsules.size(); ++i)
    {
        connect(listesDeroulantesCapsules[i],
                SIGNAL(currentIndexChanged(int)),
                this,
                SLOT(choisirCapsuleStock(int)));
    }

    // La gestion du stock
    for(int i = 0; i < stocksRangeesCapsules.size(); ++i)
    {
        connect(stocksRangeesCapsules[i],
                SIGNAL(valueChanged(int)),
                this,
                SLOT(modifierStock(int)));
    }

    // Les boutons de préparation de café
    connect(ui->boutonCafeCourt, &QPushButton::clicked, this, &IhmPikawa::preparerCafeCourt);
    connect(ui->boutonCafeLong, &QPushButton::clicked, this, &IhmPikawa::preparerCafeLong);
    connect(ui->boutonDernierCafe, &QPushButton::clicked, this, &IhmPikawa::preparerDernierCafe);
    connect(minuteurPreparationCafe,
            &QTimer::timeout,
            this,
            &IhmPikawa::mettreAJourBarreProgressionPreparationCafe);

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
}

void IhmPikawa::initialiserListeCapsules()
{
    QVector<QStringList> listeCapsules = gestionMagasin->getListeCapsules();
    QVector<QStringList> stock         = gestionMagasin->getStock();
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
        QFont formatFont = listesDeroulantesCapsules[i]->font();
        formatFont.setCapitalization(QFont::Capitalize);
        listesDeroulantesCapsules[i]->setFont(formatFont);
        listesDeroulantesCapsules[i]->addItem("aucune");

        // séléctionne la désignation de capsule actuellement dans le stock
        listesDeroulantesCapsules[i]->setCurrentText(
          stock[i].at(GestionMagasin::StockMagasin::DESIGNATION_CAPSULE_STOCK));

        if(stock[i].at(GestionMagasin::StockMagasin::DESIGNATION_CAPSULE_STOCK) != "aucune")
            stocksRangeesCapsules[i]->setEnabled(true);
        else
            stocksRangeesCapsules[i]->setEnabled(false);
    }
}

void IhmPikawa::initialiserStocksRangeeCapsules()
{
    QVector<QStringList> stock = gestionMagasin->getStock();
    for(int i = 0; i < stock.size(); ++i)
    {
        int numeroRangee = stock[i].at(GestionMagasin::StockMagasin::RANGEE_CAPSULE_STOCK).toInt();
        int quantiteRangee =
          stock[i].at(GestionMagasin::StockMagasin::QUANTITE_CAPSULE_STOCK).toInt();
        qDebug() << Q_FUNC_INFO << "numeroRangee" << numeroRangee << "quantiteRangee"
                 << quantiteRangee;
        stocksRangeesCapsules[numeroRangee - 1]->setValue(quantiteRangee);
    }
    // Initialisation du LCDNumber capsuleTotalRestante
    int capsulesRestantesTotal = calculerTotalCapsulesRestantes();
    ui->capsuleTotalRestantes->display(capsulesRestantesTotal);
    if(capsulesRestantesTotal < SEUIL_CAPSULES_RESTANTES)
        ui->capsuleTotalRestantes->setStyleSheet("background-color: transparent; color: red;");
    else
        ui->capsuleTotalRestantes->setStyleSheet("background-color: transparent;color: green;");
}

void IhmPikawa::initialiserBoutonsCapsules()
{
    qDebug() << Q_FUNC_INFO;
    for(int i = 0; i < listesDeroulantesCapsules.size(); ++i)
    {
        if(listesDeroulantesCapsules[i]->currentText() != "aucune" &&
           listeLCDNumberCapsules[i]->value() > 0)
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
    for(int i = 0; i < presenceCapsulesPikawa.size(); i++)
    {
        if(presenceCapsulesPikawa.at(i) == "0")
        {
            boutonsChoixCapsules.at(i)->setEnabled(false);
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
    QVector<QStringList> stock = gestionMagasin->getStock();
    for(int i = 0; i < stock.size(); ++i)
    {
        int numeroRangee = stock[i].at(GestionMagasin::StockMagasin::RANGEE_CAPSULE_STOCK).toInt();
        int quantiteRangee =
          stock[i].at(GestionMagasin::StockMagasin::QUANTITE_CAPSULE_STOCK).toInt();
        qDebug() << Q_FUNC_INFO << "numeroRangee" << numeroRangee << "quantiteRangee"
                 << quantiteRangee;
        listeLCDNumberCapsules[numeroRangee - 1]->display(quantiteRangee);
        stocksRangeesCapsules[numeroRangee - 1]->setValue(
          listeLCDNumberCapsules[numeroRangee - 1]->value());
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

int IhmPikawa::rechercherRangee(QSpinBox* stockRangee)
{
    if(stockRangee == nullptr)
        return 0;

    for(int i = 0; i < stocksRangeesCapsules.size(); ++i)
    {
        if(stockRangee == stocksRangeesCapsules[i])
            return i + 1;
    }
    return 0;
}

int IhmPikawa::rechercherRangee(QComboBox* listeDeroulanteCapsules)
{
    if(listeDeroulanteCapsules == nullptr)
        return 0;

    for(int i = 0; i < listesDeroulantesCapsules.size(); ++i)
    {
        if(listeDeroulanteCapsules == listesDeroulantesCapsules[i])
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

void IhmPikawa::deselectionnerAutresRangees(QPushButton* bouton)
{
    if(bouton == nullptr)
        return;

    for(int i = 0; i < boutonsChoixCapsules.size(); ++i)
    {
        if(bouton != boutonsChoixCapsules[i])
            boutonsChoixCapsules[i]->setChecked(false);
    }
}

void IhmPikawa::deselectionnerRangee(int rangee)
{
    if(rangee >= 1 && rangee <= boutonsChoixCapsules.size()) // Vérifier que la rangée est valide
    {
        boutonsChoixCapsules[rangee - 1]->setChecked(false);
    }
}

void IhmPikawa::decrementerNbCapsules()
{
    if(rangeeSelectionneePreparation >= 1 &&
       rangeeSelectionneePreparation <=
         listeLCDNumberCapsules.size()) // Vérifier que la rangée est valide
    {
        int capsulesRestantes = listeLCDNumberCapsules[rangeeSelectionneePreparation - 1]
                                  ->value(); // Récupérer le nombre de capsules restantes

        qDebug() << Q_FUNC_INFO << "rangeeSelectionneePreparation" << rangeeSelectionneePreparation
                 << "capsulesRestantes" << capsulesRestantes;
        if(capsulesRestantes > 0)
        {
            capsulesRestantes--; // Décrémenter le nombre de capsules

            listeLCDNumberCapsules[rangeeSelectionneePreparation - 1]->display(
              capsulesRestantes); // Mettre à jour l'affichage du nombre de capsules
            stocksRangeesCapsules[rangeeSelectionneePreparation - 1]->setValue(
              listeLCDNumberCapsules[rangeeSelectionneePreparation - 1]->value());

            int capsulesRestantesTotal = calculerTotalCapsulesRestantes();
            ui->capsuleTotalRestantes->display(capsulesRestantesTotal);
            if(capsulesRestantesTotal < SEUIL_CAPSULES_RESTANTES)
                ui->capsuleTotalRestantes->setStyleSheet(
                  "background-color: transparent; color: red;");
            else
                ui->capsuleTotalRestantes->setStyleSheet(
                  "background-color: transparent;color: green;");

            QString requeteSQL = "UPDATE StockMagasin SET quantite='" +
                                 QString::number(capsulesRestantes) + "' WHERE rangee='" +
                                 QString::number(rangeeSelectionneePreparation) + "'";
            bdd->executer(requeteSQL);
            rangeeSelectionneePreparation = 0;
        }
    }
}

int IhmPikawa::calculerTotalCapsulesRestantes()
{
    int quantiteCapsulesMagasin = 0;
    for(int i = 0; i < listeLCDNumberCapsules.size(); ++i)
    {
        quantiteCapsulesMagasin += listeLCDNumberCapsules[i]->value();
    }
    qDebug() << Q_FUNC_INFO << "quantiteCapsulesMagasin" << quantiteCapsulesMagasin;
    return quantiteCapsulesMagasin;
}
