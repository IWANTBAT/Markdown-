#include"markdown.hpp"


void test(){
	Markdownparser mp("test.txt");
	mp.transform();
	mp.getHtml();
}
int main()
{
	test();
	return 0;
}