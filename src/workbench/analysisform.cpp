/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
//  *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef Q_MOC_RUN
#include "base/resultset.h"
#endif

#include "analysisform.h"
#include "ui_analysisform.h"
#include "parameterwrapper.h"
#include "resultelementwrapper.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QTemporaryFile>
#include <QScrollBar>
#include "email.h"

#include "ui_xml_display.h"

int metaid1=qRegisterMetaType<insight::ParameterSet>("insight::ParameterSet");
int metaid2=qRegisterMetaType<insight::ResultSetPtr>("insight::ResultSetPtr");


AnalysisWorker::AnalysisWorker(const std::shared_ptr<insight::Analysis>& analysis)
: analysis_(analysis)
{}

void AnalysisWorker::doWork(insight::ProgressDisplayer* pd)
{
    insight::ResultSetPtr results = (*analysis_)(pd);
    emit resultReady( results );
    emit finished();
}

AnalysisForm::AnalysisForm(QWidget* parent, const std::string& analysisName)
: QMdiSubWindow(parent),
  analysisName_(analysisName),
  executionPathParameter_(".", "Directory to store data files during analysis.\nLeave empty for temporary storage.")
{

    // load default parameters
    parameters_ = insight::Analysis::defaultParameters(analysisName_);

    ui = new Ui::AnalysisForm;
    QWidget* iw=new QWidget(this);
    ui->setupUi(iw);
    setWidget(iw);

    QSplitter* spl=new QSplitter(Qt::Vertical);
    QWidget* lower = new QWidget;
    QHBoxLayout* hbl = new QHBoxLayout(lower);
    progdisp_=new GraphProgressDisplayer;
    log_=new QPlainTextEdit;
    spl->addWidget(progdisp_);
    spl->addWidget(lower); //log_);
    hbl->addWidget(log_);
    QVBoxLayout* vbl=new QVBoxLayout;
    hbl->addLayout(vbl);
    save_log_btn_=new QPushButton("Save...");
    connect(save_log_btn_, SIGNAL(clicked()), this, SLOT(saveLog()));
    send_log_btn_=new QPushButton("Email...");
    connect(send_log_btn_, SIGNAL(clicked()), this, SLOT(sendLog()));
    clear_log_btn_=new QPushButton("Clear");
    connect(clear_log_btn_, SIGNAL(clicked()), this, SLOT(clearLog()));
    auto_scroll_down_btn_=new QPushButton("Auto Scroll");
    connect(auto_scroll_down_btn_, SIGNAL(clicked()), this, SLOT(autoScrollLog()));
    vbl->addWidget(save_log_btn_);
    vbl->addWidget(send_log_btn_);
    vbl->addWidget(clear_log_btn_);
    vbl->addWidget(auto_scroll_down_btn_);

    ui->runTabLayout->addWidget(spl);
    
    cout_log_ = new Q_DebugStream(std::cout);
    connect(cout_log_, SIGNAL(appendText(const QString&)), log_, SLOT(appendPlainText(const QString&)));
    cerr_log_ = new Q_DebugStream(std::cerr);
    connect(cerr_log_, SIGNAL(appendText(const QString&)), log_, SLOT(appendPlainText(const QString&)));

    this->setWindowTitle(analysisName_.c_str());
    connect(ui->runBtn, SIGNAL(clicked()), this, SLOT(onRunAnalysis()));
    connect(ui->killBtn, SIGNAL(clicked()), this, SLOT(onKillAnalysis()));

    insight::ParameterSet_VisualizerPtr viz;
    insight::ParameterSet_ValidatorPtr vali;

    try {
        viz = insight::Analysis::visualizer(analysisName_);
    } catch (insight::Exception e)
    { /* ignore, if non-existent */ }

    try {
        vali = insight::Analysis::validator(analysisName_);
    } catch (insight::Exception e)
    { /* ignore, if non-existent */ }

    peditor_=new ParameterEditorWidget(parameters_, ui->inputTab, vali, viz);
    ui->inputTabLayout->addWidget(peditor_);
    peditor_->insertParameter("execution directory", executionPathParameter_);
    QObject::connect(this, SIGNAL(apply()), peditor_, SLOT(onApply()));
    QObject::connect(this, SIGNAL(update()), peditor_, SLOT(onUpdate()));

    rtroot_=new QTreeWidgetItem(0);
    rtroot_->setText(0, "Results");
    ui->resultTree->setColumnCount(3);
    ui->resultTree->setHeaderLabels( QStringList() << "Result Element" << "Description" << "Current Value" );
    ui->resultTree->addTopLevelItem(rtroot_);
}

AnalysisForm::~AnalysisForm()
{
    workerThread_.quit();
    workerThread_.wait();
    delete ui;
}

void AnalysisForm::insertMenu(QMenuBar* mainMenu)
{
    qDebug()<<"insertMenu";
    workbench::WidgetWithDynamicMenuEntries::insertMenu(mainMenu);

    menu_parameters_=mainMenu_->addMenu("&Parameters");
    if (!act_save_as_) act_save_as_=new QAction("&Save parameter set as...", this);
    menu_parameters_->addAction( act_save_as_ );
    connect( act_save_as_, SIGNAL(activated()), this, SLOT(onSaveParameters()) );
    if (!act_merge_) act_merge_=new QAction("&Merge other parameter set into current...", this);
    menu_parameters_->addAction( act_merge_ );
    connect( act_merge_, SIGNAL(activated()), this, SLOT(onLoadParameters()) );
    if (!act_param_show_) act_param_show_=new QAction("&Show in XML format", this);
    menu_parameters_->addAction( act_param_show_ );
    connect( act_param_show_, SIGNAL(activated()), this, SLOT(onShowParameterXML()) );


    menu_actions_=mainMenu_->addMenu("&Actions");

    if (!act_run_) act_run_=new QAction("&Run Analysis", this);
    menu_actions_->addAction( act_run_ );
    connect( act_run_, SIGNAL(activated()), this, SLOT(onRunAnalysis()) );
    if (!act_kill_) act_kill_=new QAction("&Stop Analysis", this);
    menu_actions_->addAction( act_kill_ );
    connect( act_kill_, SIGNAL(activated()), this, SLOT(onKillAnalysis()) );

    menu_results_=mainMenu_->addMenu("&Results");

    if (!act_save_rpt_) act_save_rpt_=new QAction("Create &report...", this);
    menu_results_->addAction( act_save_rpt_ );
    connect( act_save_rpt_, SIGNAL(activated()), this, SLOT(onCreateReport()) );

}

void AnalysisForm::removeMenu()
{
    if (mainMenu_)
    {
        qDebug()<<"removeMenu";
        menu_parameters_->removeAction(act_save_as_); act_save_as_->disconnect(); //act_save_as_->deleteLater();
        menu_parameters_->removeAction(act_merge_); act_merge_->disconnect(); //act_merge_->deleteLater();
        menu_parameters_->removeAction(act_param_show_); act_param_show_->disconnect(); //act_param_show_->deleteLater();

        menu_actions_->removeAction(act_run_); act_run_->disconnect(); //act_run_->deleteLater();
        menu_actions_->removeAction(act_kill_); act_kill_->disconnect(); //act_kill_->deleteLater();

        QAction *ma;
        ma = menu_results_->menuAction();
        ma->disconnect();
        mainMenu_->removeAction(ma);
        //menu_results_->deleteLater();

        ma = menu_parameters_->menuAction();
        ma->disconnect();
        mainMenu_->removeAction(ma);
        //menu_parameters_->deleteLater();

        ma = menu_actions_->menuAction();
        ma->disconnect();
        mainMenu_->removeAction(ma);
        //menu_actions_->deleteLater();
    }
    workbench::WidgetWithDynamicMenuEntries::removeMenu();
}

void AnalysisForm::closeEvent(QCloseEvent * event)
{
    QMdiSubWindow::closeEvent(event);
    if (event->isAccepted()) removeMenu();
}

void AnalysisForm::onSaveParameters()
{
//   emit apply();

  QString fn = QFileDialog::getSaveFileName(this, "Save Parameters", QString(), "Insight parameter sets (*.ist)");
  if (!fn.isEmpty())
  {
//     parameters_.saveToFile(fn.toStdString(), analysis_->type());
    parameters_.saveToFile(fn.toStdString(), analysisName_);
  }
}

void AnalysisForm::onLoadParameters()
{
  QString fn = QFileDialog::getOpenFileName(this, "Open Parameters", QString(), "Insight parameter sets (*.ist)");
  if (!fn.isEmpty())
  {
    parameters_.readFromFile(fn.toStdString());
    emit update();
  }
}

void AnalysisForm::onShowParameterXML()
{
    QDialog *widget = new QDialog(this);
    Ui::XML_Display ui;
    ui.setupUi(widget);

    emit apply(); // apply all changes into parameter set
    boost::filesystem::path exePath = executionPathParameter_();
    std::ostringstream os;
    parameters_.saveToStream(os, exePath, analysisName_);
    ui.textDisplay->setText(QString::fromStdString(os.str()));

    widget->exec();
}

void AnalysisForm::onRunAnalysis()
{
    if (!workerThread_.isRunning())
    {

        if (analysis_ || results_)
        {
            QMessageBox msgBox;
            if (results_)
            {
                msgBox.setText("There is currently a result set in memory!");
            } else {
                msgBox.setText("There is currently an analysis open.");
            }
            msgBox.setInformativeText("If you continue, the results will be deleted and the execution directory on disk will be removed (only if it was created). Continue?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);


            if (msgBox.exec()==QMessageBox::Yes)
            {
                results_.reset();
                analysis_.reset();
            } else {
                return;
            }
        }

        emit apply(); // apply all changes into parameter set
        boost::filesystem::path exePath = executionPathParameter_();

        analysis_.reset( insight::Analysis::lookup(analysisName_, parameters_, exePath) );

        progdisp_->reset();

        AnalysisWorker *worker = new AnalysisWorker(analysis_);
        worker->moveToThread(&workerThread_);
        
        connect(this, SIGNAL(runAnalysis(insight::ProgressDisplayer*)),
                worker, SLOT(doWork(insight::ProgressDisplayer*)));
        connect(worker, SIGNAL(resultReady(insight::ResultSetPtr)), this, SLOT(onResultReady(insight::ResultSetPtr)));
        
        connect(worker, SIGNAL(finished()), &workerThread_, SLOT(quit()));
        connect(&workerThread_, SIGNAL(finished()), worker, SLOT(deleteLater()));
        workerThread_.start();

        ui->tabWidget->setCurrentWidget(ui->runTab);

        emit runAnalysis(progdisp_);
    }
}

void AnalysisForm::onKillAnalysis()
{
  if (workerThread_.isRunning())
  {
    analysis_->cancel();
    workerThread_.quit();
    workerThread_.wait();
  }
}

void AnalysisForm::onResultReady(insight::ResultSetPtr results)
{
  results_=results;
  
//   qDeleteAll(ui->outputContents->findChildren<ResultElementWrapper*>());
//   addWrapperToWidget(*results_, ui->outputContents, this);

  rtroot_->takeChildren();
  addWrapperToWidget(*results_, rtroot_, this);
  ui->resultTree->doItemsLayout();
  ui->resultTree->expandAll();
  ui->resultTree->resizeColumnToContents(2);

  ui->tabWidget->setCurrentWidget(ui->outputTab);

  QMessageBox::information(this, "Finished!", "The analysis has finished");
}

void AnalysisForm::onCreateReport()
{
  if (!results_.get())
  {
    QMessageBox::critical(this, "Error", "No results present!");
    return;
  }
  
  QString fn = QFileDialog::getSaveFileName
  (
      this, 
    "Save Report",
    QString(executionPathParameter_().c_str()), 
    "PDF file (*.pdf)|LaTeX file (*.tex)"
  );
  if (!fn.isEmpty())
  {
    boost::filesystem::path outpath=fn.toStdString();
    std::string ext=outpath.extension().string();

    if (boost::algorithm::to_lower_copy(ext)==".tex")
      {
        results_->writeLatexFile( outpath );
      }
    else if (boost::algorithm::to_lower_copy(ext)==".pdf")
      {
        results_->generatePDF( outpath );
      }
    else
      {
        QMessageBox::critical(this, "Error!", "Unknown file format: "+fn);
        return;
      }

    QMessageBox::information(this, "Done!", QString("The report has been created as\n")+outpath.c_str());
  }
}

void AnalysisForm::saveLog()
{
    QString fn=QFileDialog::getSaveFileName(
        this, 
        "Save Log to file",
        "",
        "*.txt"
    );
    
    if (!fn.isEmpty())
    {
        QFile f(fn);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox::critical(this, "Create file failed", "Could not create file "+fn);
            return;
        }
        QTextStream out(&f);
        out << log_->toPlainText();
    }
}

void AnalysisForm::sendLog()
{
    QTemporaryFile f;
    if (!f.open())
    {
        QMessageBox::critical(this, "Creation of temporary file failed", 
                              "Could not create temporary file "+f.fileName());
        return;
    }
    QTextStream out(&f);
    out<< log_->toPlainText();
    out.flush();
    
    Email e;
    e.setReceiverAddress("info@silentdynamics.de");
    e.setSubject("Analysis Log");
    e.addAttachment(QFileInfo(f).canonicalFilePath());
    e.openInDefaultProgram();
}


void AnalysisForm::clearLog()
{
  log_->clear();
}

void AnalysisForm::autoScrollLog()
{
  log_->verticalScrollBar()->setValue( log_->verticalScrollBar()->maximum() );
}
