#include "Operator.h"
#include "qdebug.h"
#include "tcpclient.h"
#include <iostream>
using namespace std;


FieldOperator::FieldOperator(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	this->setWindowTitle("ObsOperator v0.9");

	//connect(ui.pingButton, &QToolButton::clicked, this, &FieldOperator::onPingButtonClicked);
	connect(ui.sceneSelect, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &FieldOperator::onSceneSelectActivated);
	connect(ui.plainTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(logScroll()));
	//connect(ui.plainTextEdit, SIGNAL(cursorPositionChanged()), this, &FieldOperator::logScroll);
}

void FieldOperator::onPingButtonClicked()
{
	qDebug() << "onPingButtonClicked() is clicked.";
	QString svrHost = ui.svrTcpHost->text();
	int svrPort = ui.svrTcpPort->text().toInt();
	char res[100] = {0};
	if (svrHost.length() > 4 && svrPort > 1000) {
		int len = TcpSendPingMsg(svrHost.toLatin1().data(), svrPort, res);
		ui.plainTextEdit->textCursor().insertText("send:ping\n");
		if (len > 0) {
			QString qRes(res);
			ui.plainTextEdit->textCursor().insertText("recv:" + qRes);

		}
	}
	
}

void FieldOperator::onSceneSelectActivated(int index)
{
	//qDebug() << "onSceneSelectActivated(). index=" << index << ",name=" << ui.sceneSelect->currentText();
	printf("onSceneSelectActivated(). index=%d,name=%s\n",index, ui.sceneSelect->currentText().toLatin1().data());

	QString svrHost = ui.svrTcpHost->text();
	int svrPort = ui.svrTcpPort->text().toInt();
	char res[1000] = { 0 };
	if (svrHost.length() > 4 && svrPort > 1000) {
		int len = TcpSendSceneMsg(svrHost.toLatin1().data(), svrPort, ui.devId->text().toStdString(), ui.streamId->text().toStdString(),ui.sceneSelect->currentText().toStdString(),res);
		ui.plainTextEdit->textCursor().insertText("send:scene-cmd. switch to "+ ui.sceneSelect->currentText() + "\n");
		if (len > 0) {
			QString qRes(res);
			ui.plainTextEdit->textCursor().insertText("recv:" + qRes);

		}
	}

}

void FieldOperator::logScroll()
{
	QTextCursor cursor = ui.plainTextEdit->textCursor();
	cursor.movePosition(QTextCursor::End);
	ui.plainTextEdit->setTextCursor(cursor);
}

