#include "xoPrimitiveConsole.h"
#include "Core.h"
#include <QGridLayout>
#include <QLabel>
#include <QTextEdit>
#include "ui/GlobalConsoleWidget.h"

xoPrimitiveConsole::xoPrimitiveConsole(QWidget *parent) : QWidget(parent)
{
      auto grid = new QGridLayout(this);

      auto console = new GlobalConsoleWidget();

      grid->addWidget(console);
}
