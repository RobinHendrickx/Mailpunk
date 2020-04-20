#include "imap.hpp"
#include "UI.hpp"
#include <memory>

int main(int argc, char** argv) {
	auto elements = std::make_unique<UI>(argc, argv);
	return elements->exec();
}
