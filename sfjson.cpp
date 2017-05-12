#include "sfjson.h"

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
		n->printTo(str);

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