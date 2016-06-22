#include "nana/gui/wvl.hpp"
#include "nana/gui/widgets/label.hpp"

#pragma comment(lib, "nana_v140_Debug_x86.lib")

int main()
{
	using namespace nana;
	form	fm;

	label	lb(fm, rectangle(fm.size()));
	lb.caption("Hello, World!");
	fm.show();
	exec();
}