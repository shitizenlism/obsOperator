#pragma once

#include <QtWidgets/QMainWindow>
#include <qtoolbutton.h>
#include <qtexttable.h>
#include "ui_Operator.h"

class FieldOperator : public QMainWindow
{
	Q_OBJECT

public:
	FieldOperator(QWidget *parent = Q_NULLPTR);

private slots:
	void onPingButtonClicked();
	void onSceneSelectActivated(int index);
	void logScroll();


private:
	Ui::OperatorClass ui;
};
