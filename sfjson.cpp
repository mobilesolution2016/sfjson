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

		n = sfJsonCreate(true);
		n->addValue()->name("name")->val("xiaomi");
		n->addArray("haha");
		n->addArray()->name("values")->append((int64_t)1)->append((int64_t)2)->append(true)->append(3.5);

		sfNode* lv2 = n->createObject("strs");
		lv2->addValue()->name("hello")->val("hi");
		n->add(lv2);

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