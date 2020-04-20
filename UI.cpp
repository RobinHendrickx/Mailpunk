#include "UI.hpp"

using namespace std;
using namespace finalcut;
void UI::refreshMailList() {
	auto oldMailDialog = mailDialog;
	mailDialog = new FDialog(app);
	app->setMainWidget(mailDialog);
	app->setActiveWindow(mailDialog);
	if(oldMailDialog)
		oldMailDialog->close();

	mailDialog->setText("INBOX");
	mailDialog->zoomWindow();

	auto oldmailListView = mailListView;
	// mailListView->unsetFocus();
	mailListView = new FListView(mailDialog);
	mailListView->setGeometry(mailDialog->getGeometry());
	mailListView->addColumn("From");
	mailListView->addColumn("Subject");
	auto messages = imapSession->getMessages();
	if(!viewToMessageMap)
		viewToMessageMap = new map<FObject*, IMAP::Message*>();
	viewToMessageMap->clear();

	for(size_t i = 0; messages[i]; i++) {
		(*(viewToMessageMap))[*mailListView->insert(
				{messages[i]->getField("From"), messages[i]->getField("Subject")})] = messages[i];
	}
	mailListView->addCallback("clicked", [](FWidget* view, void* ptr) {
		auto elements = static_cast<UI*>(ptr);
		auto listView = static_cast<FListView*>(view);
		auto item = listView->getCurrentItem();
		auto message = (*(elements->viewToMessageMap))[item];
		auto body = message->getBody();
		FMessageBox::info(elements->mailDialog, listView->getCurrentItem()->getText(2), body);
	}, this);
	mailDialog->activateDialog();
	mailListView->setFocus();
	mailDialog->show();
	app->redraw();
}

void UI::loginClicked(FWidget*) {
	auto elements = this;
	auto& session =
			*(elements->imapSession = new IMAP::Session([elements]() { elements->refreshMailList(); }));
	try {
		session.connect(elements->inputFields["server"]->getText().toString());
		session.login(elements->inputFields["user"]->getText().toString(),
									elements->inputFields["password"]->getText().toString());
		session.selectMailbox("INBOX");
	} catch(runtime_error const& exception) {
		FMessageBox::info(elements->initDialog, "Error", exception.what());
		return;
	}

	elements->initDialog->hide();
	elements->initDialog->unsetFocus();
	// delCallback(nullptr)
	elements->refreshMailList();

	auto deleteKey = new FStatusKey(fc::Fckey_d, "Delete", elements->statusBar);
	deleteKey->addCallback("activate", [](auto*, auto* _) {
		auto elements = static_cast<UI*>(_);
		auto item = elements->mailListView->getCurrentItem();
		auto message = (*(elements->viewToMessageMap))[item];
		message->deleteFromMailbox();

	}, elements);
	elements->app->redraw();
}

void UI::quitKeyActivated(FWidget*) {
	if(initDialog)
		initDialog->close();

	if(mailDialog)
		mailDialog->close();
};

int UI::exec() {
	auto elements = this;
	app = new FApplication(argc, argv);
	initDialog = new FDialog(app);
	auto status = statusBar = new FStatusBar(initDialog);
	auto quitKey = new FStatusKey(fc::Fmkey_x, "Quit", status);
	quitKey->addCallback("activate", F_METHOD_CALLBACK(elements, &UI::quitKeyActivated)
											 // F_METHOD_CALLBACK(app, &FApplication::cb_exitApp), nullptr
											 );
	// app->setActiveWindow(initDialog);
	app->setMainWidget(initDialog);

	// status->setMessage("login");
	// status->show();
	// status->drawMessage();
	// status->setAlwaysOnTop();

	initDialog->setGeometry(4, 3, 41, 11);
	inputFields.emplace("user", new FLineEdit(initDialog));
	inputFields.emplace("server", new FLineEdit(initDialog));
	inputFields.emplace("password", new FLineEdit(initDialog));
	size_t i = 0;
	for(auto field = inputFields.rbegin(); field != inputFields.rend(); ++field) {
		field->second->unsetShadow();
		auto env = field->first;
		field->second->setLabelText("&"s + field->first);
		field->second->setGeometry(15, 1 + 2 * i++, 19, 1);
		field->second->addCallback("activate", [](FWidget* widget, auto*) {
			static FKeyEvent e(1, fc::Fkey_tab);
			FApplication::sendEvent(widget, &e);
		});

		for(auto& c : env)
			c = toupper(c);
		field->second->setText(getenv(env.c_str()));
	}
	inputFields["password"]->setFocus();
	auto login = loginButton = new FButton("&Login", initDialog);
	login->setGeometry(15, 7, 19, 1);
	login->unsetShadow();
	login->addCallback("clicked", F_METHOD_CALLBACK(elements, &UI::loginClicked));
	initDialog->show();
	return app->exec();
}

UI::~UI() {
	delete imapSession;
	delete mailDialog;
	// delete mailListView;
	delete app;
	delete viewToMessageMap;
}
