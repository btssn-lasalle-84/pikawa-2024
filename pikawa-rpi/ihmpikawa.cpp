#include "ihmpikawa.h"
#include "ui_ihmpikawa.h"
#include "GestionMachine.h"
#include "BaseDeDonnees.h"
#include <QDebug>

/**
 * @file ihmpikawa.cpp
 *
 * @brief Définition de la classe IhmPikawa
 * @author MDOIOUHOMA Nakib
 * @version 0.1
 */

/**
 * @brief Constructeur de la classe IhmPikawa
 *
 * @fn IhmPikawa::IhmPikawa
 * @param parent L'adresse de l'objet parent, si nullptr IHMPikawa sera la
 * fenêtre principale de l'application
 */
IhmPikawa::IhmPikawa(QWidget* parent) :
    QMainWindow(parent), ui(new Ui::IhmPikawa), gestionMachine(new GestionMachine(this)),
    bdd(BaseDeDonnees::getInstance())
{
    qDebug() << Q_FUNC_INFO;
    ui->setupUi(this);

    fixerRaccourcisClavier();
    gererEvenements();

    changerEcranAccueil();
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

void IhmPikawa::changerEcranCafe()
{
    afficherEcran(IhmPikawa::Ecran::EcranCafe);
}

void IhmPikawa::changerEcranMachine()
{
    afficherEcran(IhmPikawa::Ecran::EcranMachine);
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
    connect(ui->selectionEcranCafe, &QPushButton::clicked, this, &IhmPikawa::changerEcranCafe);
    connect(ui->retourAccueilDeCafe, &QPushButton::clicked, this, &IhmPikawa::changerEcranAccueil);
    connect(ui->selectionEcranMachine,
            &QPushButton::clicked,
            this,
            &IhmPikawa::changerEcranMachine);
}
