// @HEADER
// ***********************************************************************
// 
//         Optika: A Tool For Developing Parameter Obtaining GUIs
//                Copyright (2009) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, with Sandia Corporation, the 
// U.S. Government retains certain rights in this software.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Kurtis Nusbaum (klnusbaum@gmail.com) 
// 
// ***********************************************************************
// @HEADER

#include <QtGui>
#include <QtTest/QtTest>
#include "Teuchos_UnitTestHarness.hpp"
#include "Optika_treemodel.hpp"
#include "Optika_delegate.hpp"
#include "Optika_treeview.hpp"
#include <QApplication>
#include <QSpinBox>
#include <iostream>

/**
 *  THIS IS JUST A PROOF OF CONCEPT!
 *  I just wanted to see if I could get the QTest
 *  framework working with our CTest Framework.
 *  I don't have time right now to actually
 *  do extensive GUI unit testing. I will come
 *  back and actually write the tests a.s.a.p.
 *
 *  kln 21/01/2011
 */
namespace Optika{

class OptikaGUITests: public QObject{
Q_OBJECT
private slots:
  void typeTest();
  void dependencyTests();
private:
};

//QModelIndex OptikaGUITests::getEntryIndex(

#define GET_ENTRY_INDEX(\
  PL, \
  NAME, \
  MODEL) \
  RCP<ParameterEntry> NAME##Entry = PL->getEntryRCP( #NAME  ); \
  QVERIFY(nonnull( NAME##Entry )); \
  QModelIndex NAME##Index = MODEL->findParameterEntryIndex( NAME##Entry ); \
  QVERIFY( NAME##Index.isValid());

/*
void OptikaGUITests::verifyParameterAndType(
  RCP<ParameterList> pl,
  QString name, 
  QString type, 
  TreeModel* model)
{
  QModelIndex itemIndex = getEntryIndex(pl, name.toStdString(), model);
  QCOMPARE(model->data(itemIndex, Qt::DisplayRole).toString(),name);
  QModelIndex typeIndex = itemIndex.sibling(itemIndex.row(),2);
  QVERIFY(typeIndex.isValid());
  QCOMPARE(model->data(typeIndex, Qt::DisplayRole).toString(),type);
}*/

#define VERIFY_PARAMETER_TYPE(PL, NAME, TYPE, MODEL) \
  GET_ENTRY_INDEX( PL , NAME , MODEL ) \
  QCOMPARE( MODEL->data( NAME##Index, Qt::DisplayRole).toString(), \
    QString::fromStdString( #NAME) );  \
  QModelIndex NAME##TypeIndex = NAME##Index.sibling(NAME##Index.row(),2); \
  QVERIFY( NAME##TypeIndex.isValid()); \
  QCOMPARE( MODEL->data( NAME##TypeIndex, Qt::DisplayRole).toString(), TYPE );


void OptikaGUITests::typeTest(){
  RCP<ParameterList> My_List = 
    RCP<ParameterList>(new ParameterList);

  double *pointer = 0;
  My_List->set("Doublepointer", pointer);
  My_List->set(
    "MaxIters", 
    1550, 
    "Determines the maximum number of iterations in the solver");
  My_List->set(
    "Tolerance", 1e-10, "The tolerance used for the convergence check");
  
  TreeModel* model = new TreeModel(My_List);

  VERIFY_PARAMETER_TYPE(My_List, MaxIters, intId, model)
  VERIFY_PARAMETER_TYPE(My_List, Doublepointer, unrecognizedId, model);
  VERIFY_PARAMETER_TYPE(My_List, Tolerance, doubleId, model);
}


void OptikaGUITests::dependencyTests(){
  RCP<DependencySheet> dependencySheet = rcp(new DependencySheet);
  RCP<ParameterList> validParameters = 
    getParametersFromXmlFile("deptests.xml", dependencySheet);
  TreeModel* model = new TreeModel(validParameters, dependencySheet);
  Delegate* delegate = new Delegate;
  TreeView* treeView = new TreeView(model, delegate);
  QStyleOptionViewItem genericStyleItem;

  GET_ENTRY_INDEX(validParameters, Preconditioner, model)
  GET_ENTRY_INDEX(validParameters, ShowPrecs, model);

  QVERIFY(treeView->isRowHidden(
    PreconditionerIndex.row(), PreconditionerIndex.parent()));
  QModelIndex precWidgetIndex = ShowPrecsIndex.sibling(ShowPrecsIndex.row(),1);
  QComboBox* precBox = (QComboBox*)delegate->createEditor(
    0, genericStyleItem, precWidgetIndex);
  precBox->setCurrentIndex(precBox->findText("true"));
  delegate->setModelData(precBox, model, ShowPrecsIndex);
  QVERIFY(!treeView->isRowHidden(
    PreconditionerIndex.row(), PreconditionerIndex.parent()));

  




}
  


} //namespace Optika

/**
 *  Test the basics of Optika Functionality
 */
/*TEUCHOS_UNIT_TEST(Optika_GUITests, testBasics){
  int argc = 0;
  char* argv[1];
  char name [] = "optika_basic_test";
  argv[0]=name;
  QApplication app(argc, argv);
  QTEST_DISABLE_KEYPAD_NAVIGATION
  Optika::BasicTests bt;
  TEUCHOS_ASSERT(QTest::qExec(&bt, argc, argv) == 0);
}*/

QTEST_MAIN(Optika::OptikaGUITests)
#include "GUI_UnitTests.moc"

