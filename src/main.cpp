#include "../include/Scop.hpp"

int main()
{
	try {
		Scop scop;

		scop.run();
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
