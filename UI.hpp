#ifndef UI_H
#define UI_H
#include "imap.hpp"
#include <final/final.h>

struct UI {
	int argc;
	char** argv;
	UI(int argc, char** argv) : argc(argc), argv(argv) {}
	std::map<std::string, finalcut::FLineEdit*> inputFields{};
	finalcut::FApplication* app{};
	finalcut::FDialog* initDialog{};
	finalcut::FListView* mailListView{};
	finalcut::FDialog* mailDialog{};
	std::map<finalcut::FObject*, IMAP::Message*>* viewToMessageMap{};
	finalcut::FButton* loginButton{};
	finalcut::FStatusBar* statusBar{};
	IMAP::Session* imapSession{};
	void refreshMailList();
	void loginClicked(finalcut::FWidget*);
	void quitKeyActivated(finalcut::FWidget*);
	void loginFieldActivated() {}
	int exec();
	~UI();
};

#endif /* UI_H */
