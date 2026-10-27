// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <memory>

#include <QLabel>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>

#include "certificateactions.h"
#include "certificatesmodel.h"
#include "certificatespage.h"
#include "messagedialog.h"
#include "modelactions.h"
#include "namespacespage.h"
#include "pki/certificategenerator.h"
#include "server/servercontroller.h"
#include "testproject.h"

namespace {

QString testData(const QString &name)
{
    return QStringLiteral(OUASIM_TEST_DATA_DIR "/") + name;
}

MessageDialog *openDialog(const QWidget &window)
{
    return window.findChild<MessageDialog *>();
}

void click(MessageDialog *dialog, const char *button)
{
    dialog->findChild<QPushButton *>(QLatin1String(button))->click();
}

using Category = CertificateInfo::Category;

}

class TestActions : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void certificateImportFailureShowsWarning();
    void serverCertificateInstallFailureShowsWarning();
    void importedCertificateReachesPage();
    void droppedFilesReportFailuresAtOnce();
    void droppedFilesImportSilently();
    void deletesOnlyConfirmedCertificate();
    void unknownCertificateAsksNothing();
    void generateAsksBeforeReplacing();

    void modelImportFailureShowsWarning();
    void standardNamespaceExportShowsWarning();
    void removesModelOnlyAfterConfirmation();
    void unknownModelRemovalShowsWarning();

private:
    std::unique_ptr<QTemporaryDir> _directory;
    std::unique_ptr<ServerController> _controller;
    std::unique_ptr<QWidget> _window;
};

void TestActions::init()
{
    _directory = std::make_unique<QTemporaryDir>();
    QVERIFY(_directory->isValid());
    _controller = std::make_unique<ServerController>();
    _controller->pki().store().setRoot(_directory->path());
    _controller->setProject(TestProject::create());
    _window = std::make_unique<QWidget>();
}

void TestActions::cleanup()
{
    _window.reset();
    _controller.reset();
    _directory.reset();
}

void TestActions::certificateImportFailureShowsWarning()
{
    CertificatesPage page;
    CertificateActions actions(*_controller, &page, _window.get());

    QVERIFY(!actions.importCertificateFrom(testData(QStringLiteral("pki/missing.der")), Category::Trusted));
    MessageDialog *dialog = openDialog(*_window);
    QVERIFY(dialog);
    QCOMPARE(dialog->windowTitle(), QStringLiteral("Certificate not imported"));
}

void TestActions::serverCertificateInstallFailureShowsWarning()
{
    CertificatesPage page;
    CertificateActions actions(*_controller, &page, _window.get());

    QVERIFY(!actions.installServerCertificateFrom(testData(QStringLiteral("pki/server.cert.pem")),
                                                  testData(QStringLiteral("pki/missing.key.pem"))));
    MessageDialog *dialog = openDialog(*_window);
    QVERIFY(dialog);
    QCOMPARE(dialog->windowTitle(), QStringLiteral("Server certificate not installed"));
    QVERIFY(!_controller->pki().store().hasServerCertificate());
}

void TestActions::importedCertificateReachesPage()
{
    CertificatesPage page;
    CertificateActions actions(*_controller, &page, _window.get());

    QVERIFY(actions.importCertificateFrom(testData(QStringLiteral("pki/client.der")), Category::Trusted));
    QVERIFY(!openDialog(*_window));
    QCOMPARE(page.certificatesModel()->rowCount(), 1);
}

void TestActions::droppedFilesReportFailuresAtOnce()
{
    CertificatesPage page;
    CertificateActions actions(*_controller, &page, _window.get());

    emit page.filesDropped({ testData(QStringLiteral("pki/client.der")), testData(QStringLiteral("pki/missing.der")),
                             testData(QStringLiteral("pki/other-missing.der")) },
                           Category::Issuer);
    QCOMPARE(_controller->pki().certificates().size(), 1);
    QCOMPARE(_controller->pki().certificates().first().category, Category::Issuer);
    const QList<MessageDialog *> dialogs = _window->findChildren<MessageDialog *>();
    QCOMPARE(dialogs.size(), 1);
    QCOMPARE(dialogs.first()->windowTitle(), QStringLiteral("Certificates not imported"));
    const QString text = dialogs.first()->findChild<QLabel *>(QStringLiteral("textLabel"))->text();
    QVERIFY(text.contains(QStringLiteral("missing.der")));
    QVERIFY(text.contains(QStringLiteral("other-missing.der")));
}

void TestActions::droppedFilesImportSilently()
{
    CertificatesPage page;
    CertificateActions actions(*_controller, &page, _window.get());

    QCOMPARE(actions.importCertificateFiles({ testData(QStringLiteral("pki/client.der")),
                                              testData(QStringLiteral("pki/stranger.der")) },
                                            Category::Trusted),
             2);
    QVERIFY(!openDialog(*_window));
    QCOMPARE(page.certificatesModel()->rowCount(), 2);
}

void TestActions::deletesOnlyConfirmedCertificate()
{
    CertificatesPage page;
    CertificateActions actions(*_controller, &page, _window.get());
    QVERIFY(actions.importCertificateFrom(testData(QStringLiteral("pki/client.der")), Category::Trusted));
    QVERIFY(actions.importCertificateFrom(testData(QStringLiteral("pki/stranger.der")), Category::Rejected));
    QString trusted;
    for (const CertificateInfo &info : _controller->pki().certificates()) {
        if (info.category == Category::Trusted)
            trusted = info.filePath;
    }

    emit page.deleteRequested(trusted);
    MessageDialog *question = openDialog(*_window);
    QVERIFY(question);
    QCOMPARE(question->windowTitle(), QStringLiteral("Delete Certificate"));
    click(question, "cancelButton");
    QCOMPARE(_controller->pki().certificates().size(), 2);

    QTRY_VERIFY(!openDialog(*_window));
    emit page.deleteRequested(trusted);
    click(openDialog(*_window), "actionButton");
    const QList<CertificateInfo> remaining = _controller->pki().certificates();
    QCOMPARE(remaining.size(), 1);
    QCOMPARE(remaining.first().category, Category::Rejected);
}

void TestActions::unknownCertificateAsksNothing()
{
    CertificatesPage page;
    CertificateActions actions(*_controller, &page, _window.get());

    emit page.deleteRequested(_directory->filePath(QStringLiteral("unknown.der")));
    QVERIFY(!openDialog(*_window));
}

void TestActions::generateAsksBeforeReplacing()
{
    if (!CertificateGenerator::isAvailable())
        QSKIP("This build cannot generate certificates");
    CertificatesPage page;
    CertificateActions actions(*_controller, &page, _window.get());

    emit page.generateRequested();
    QVERIFY(!openDialog(*_window));
    QVERIFY(_controller->pki().store().hasServerCertificate());
    const QByteArray generated = _controller->pki().store().material().certificate;

    emit page.generateRequested();
    MessageDialog *question = openDialog(*_window);
    QVERIFY(question);
    QCOMPARE(question->windowTitle(), QStringLiteral("Generate Server Certificate"));
    click(question, "cancelButton");
    QCOMPARE(_controller->pki().store().material().certificate, generated);

    QTRY_VERIFY(!openDialog(*_window));
    emit page.generateRequested();
    click(openDialog(*_window), "actionButton");
    QVERIFY(_controller->pki().store().material().certificate != generated);
}

void TestActions::modelImportFailureShowsWarning()
{
    NamespacesPage page;
    ModelActions actions(*_controller, &page, _window.get());

    QVERIFY(!actions.importModelFrom(testData(QStringLiteral("missing.NodeSet2.xml"))));
    MessageDialog *dialog = openDialog(*_window);
    QVERIFY(dialog);
    QCOMPARE(dialog->windowTitle(), QStringLiteral("Model not imported"));
    QVERIFY(_controller->project().nodeSetFiles.isEmpty());
}

void TestActions::standardNamespaceExportShowsWarning()
{
    NamespacesPage page;
    ModelActions actions(*_controller, &page, _window.get());

    const QString standard = _controller->snapshot().namespaces.value(0).uri;
    QVERIFY(!actions.exportNamespaceTo(_directory->filePath(QStringLiteral("ns0.xml")), standard));
    MessageDialog *dialog = openDialog(*_window);
    QVERIFY(dialog);
    QCOMPARE(dialog->windowTitle(), QStringLiteral("Namespace not exported"));
}

void TestActions::removesModelOnlyAfterConfirmation()
{
    NamespacesPage page;
    ModelActions actions(*_controller, &page, _window.get());
    const QString mixer = testData(QStringLiteral("mixer.NodeSet2.xml"));
    QVERIFY(actions.importModelFrom(mixer));
    QVERIFY(!openDialog(*_window));

    emit page.modelRemovalRequested(mixer);
    MessageDialog *question = openDialog(*_window);
    QVERIFY(question);
    QCOMPARE(question->windowTitle(), QStringLiteral("Remove Model"));
    click(question, "cancelButton");
    QCOMPARE(_controller->project().nodeSetFiles.size(), 1);

    QTRY_VERIFY(!openDialog(*_window));
    emit page.modelRemovalRequested(mixer);
    click(openDialog(*_window), "actionButton");
    QVERIFY(_controller->project().nodeSetFiles.isEmpty());
}

void TestActions::unknownModelRemovalShowsWarning()
{
    NamespacesPage page;
    ModelActions actions(*_controller, &page, _window.get());

    emit page.modelRemovalRequested(testData(QStringLiteral("pump.NodeSet2.xml")));
    click(openDialog(*_window), "actionButton");
    QTRY_VERIFY(openDialog(*_window) && openDialog(*_window)->windowTitle() == QStringLiteral("Model not removed"));
}

QTEST_MAIN(TestActions)
#include "test_actions.moc"
