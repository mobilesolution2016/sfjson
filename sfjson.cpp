#include "sfjson.h"

// simple test
struct Test {
	Test()
	{
		FILE* fp = fopen("d://1.json", "rb");
		fseek(fp, 0L, SEEK_END);
		size_t s = ftell(fp);
		fseek(fp, 0L, SEEK_SET);

		char* mem = (char*)malloc(s + 1);
		fread(mem, 1, s, fp);
		fclose(fp);

		std::string str;
		sfNode* n = sfJsonDecode(mem, s);
		//n->printTo(str);

		n = sfJsonCreate(true);
		n->add(
			n->createArray("group1")->append((int)1)->append(3.5f)->append(true)->append()
		)->add(
			n->createObject("group2")->appendNamed("a", 3.5)->appendNamed("b", false)->appendNamed("c")->add(
				n->createArray("last")->add(n->createValue()->val("Hello World!"))
			)
		);

		n->printTo(str);

		fp = fopen("d://2.json", "wb");
		fwrite(str.c_str(), 1, str.length(), fp);
		fclose(fp);

		free(mem);

		int l = 0;
	}
} _gTest;

#include <Windows.h>

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)
{
	return 0;
}