#include "Operator.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	FieldOperator w;
	w.show();
	return a.exec();
}
