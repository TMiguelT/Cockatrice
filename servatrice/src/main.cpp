/***************************************************************************
 *   Copyright (C) 2008 by Max-Wilhelm Bruker   *
 *   brukie@laptop   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QCoreApplication>
#include <QTextCodec>
#include <iostream>
#include <QMetaType>
#include <QSettings>
#include "servatrice.h"
#include "server_logger.h"
#include "rng_sfmt.h"
#ifdef Q_OS_UNIX
#include <signal.h>
#endif

RNG_Abstract *rng;
ServerLogger *logger;
ServerLoggerThread *loggerThread;

void testRNG()
{
	const int n = 500000;
	std::cerr << "Testing random number generator (n = " << n << " * bins)..." << std::endl;
	
	const int min = 1;
	const int minMax = 2;
	const int maxMax = 10;
	
	QVector<QVector<int> > numbers(maxMax - minMax + 1);
	QVector<double> chisq(maxMax - minMax + 1);
	for (int max = minMax; max <= maxMax; ++max) {
		numbers[max - minMax] = rng->makeNumbersVector(n * (max - min + 1), min, max);
		chisq[max - minMax] = rng->testRandom(numbers[max - minMax]);
	}
	for (int i = 0; i <= maxMax - min; ++i) {
		std::cerr << (min + i);
		for (int j = 0; j < numbers.size(); ++j) {
			if (i < numbers[j].size())
				std::cerr << "\t" << numbers[j][i];
			else
				std::cerr << "\t";
		}
		std::cerr << std::endl;
	}
	std::cerr << std::endl << "Chi^2 =";
	for (int j = 0; j < chisq.size(); ++j)
		std::cerr << "\t" << QString::number(chisq[j], 'f', 3).toStdString();
	std::cerr << std::endl << "k =";
	for (int j = 0; j < chisq.size(); ++j)
		std::cerr << "\t" << (j - min + minMax);
	std::cerr << std::endl << std::endl;
}

void myMessageOutput(QtMsgType /*type*/, const char *msg)
{
	logger->logMessage(msg);
}

#ifdef Q_OS_UNIX
void sigSegvHandler(int sig)
{
	if (sig == SIGSEGV)
		logger->logMessage("CRASH: SIGSEGV");
	else if (sig == SIGABRT)
		logger->logMessage("CRASH: SIGABRT");
	delete loggerThread;
	raise(sig);
}
#endif

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);
	app.setOrganizationName("Cockatrice");
	app.setApplicationName("Servatrice");
	
	QStringList args = app.arguments();
	bool testRandom = args.contains("--test-random");
	
	qRegisterMetaType<QList<int> >("QList<int>");
	
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
	
	QSettings *settings = new QSettings("servatrice.ini", QSettings::IniFormat);
	
	loggerThread = new ServerLoggerThread(settings->value("server/logfile").toString());
	loggerThread->start();
	loggerThread->waitForInit();
	logger = loggerThread->getLogger();
	
	qInstallMsgHandler(myMessageOutput);
#ifdef Q_OS_UNIX	
	struct sigaction hup;
	hup.sa_handler = ServerLogger::hupSignalHandler;
	sigemptyset(&hup.sa_mask);
	hup.sa_flags = 0;
	hup.sa_flags |= SA_RESTART;
	sigaction(SIGHUP, &hup, 0);
	
	struct sigaction segv;
	segv.sa_handler = sigSegvHandler;
	segv.sa_flags = SA_RESETHAND;
	sigemptyset(&segv.sa_mask);
	sigaction(SIGSEGV, &segv, 0);
	sigaction(SIGABRT, &segv, 0);
#endif
	rng = new RNG_SFMT;
	
	std::cerr << "Servatrice " << Servatrice::versionString.toStdString() << " starting." << std::endl;
	std::cerr << "-------------------------" << std::endl;
	
	if (testRandom)
		testRNG();
	
	Servatrice *server = new Servatrice(settings);
	QObject::connect(server, SIGNAL(destroyed()), &app, SLOT(quit()), Qt::QueuedConnection);
	
	std::cerr << "-------------------------" << std::endl;
	std::cerr << "Server initialized." << std::endl;
	
	int retval = app.exec();

	std::cerr << "Server quit." << std::endl;
	std::cerr << "-------------------------" << std::endl;
	
	delete rng;
	delete settings;
	delete loggerThread;

	return retval;
}
