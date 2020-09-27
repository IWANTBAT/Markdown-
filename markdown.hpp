#include<iostream>
#include<string>
#include<vector>
#include<fstream>

using namespace std;

enum Token{
	nul = 0,
	paragraph = 1,
	href = 2,
	ul = 3,
	ol = 4,
	li = 5,
	em = 6,
	strong = 7,
	hr = 8,
	image = 9,
	quote = 10,
	h1 = 11,
	h2 = 12,
	h3 = 13,
	h4 = 14,
	h5 = 15,
	h6 = 16,
	blockcode = 17,
	code = 18,
};


// HTML前置标签
const std::string frontTag[] = {
	"", "<p>", "", "<ul>", "<ol>", "<li>", "<em>", "<strong>", "<hr color=#CCCCCC size=1 />",
	"", "<blockquote>", "<h1>", "<h2>", "<h3>", "<h4>", "<h5>", "<h6>",
	"<pre><code>", "<code>" }; 
// HTML 后置标签
const std::string backTag[] = {
	"", "</p>", "", "</ul>", "</ol>", "</li>", "</em>",
	"</strong>", "", "", "</blockquote>", "</h1>", "</h2>",
	"</h3>", "</h4>", "</h5>", "</h6>", "</code></pre>", "</code>" };

struct Node{
	//语法类型
	int _type;

	//孩子结点
	vector<Node*> _child;

	//内容
	//elem[0]:需要显示的内容
	//eleme[1]:网址，路径等
	string _elem[2];

	Node(int type)
		:_type(type){

	}
};

class Markdownparser{
public:
	Markdownparser(const string& filename)
		:_root(new Node(nul))
		, _filename(filename)
	{

	}

	void transform(){
		//打开文件
		ifstream fin(_filename);
		if (!fin.is_open()){
			cout << "文件打开失败" << endl;
			return;
		}
		//读取内容（按行读取）
		string row;

		bool inBlockcode = false;//是否在代码块当中
		while (!fin.eof()){
			getline(fin, row);

			//预处理：去掉行首空格
			const char* start = processStr(row.c_str());

			//空行
			if (!inBlockcode && start == nullptr)
				continue;

			//水平分割线
			if(!inBlockcode && isCutLine(start))
			{
				_root->_child.push_back(new Node(hr));
				continue;
			}
			//语法解析->行首语法
			pair<int, const char*> typeRet = praseType(start);

			//创建语法节点

			//代码块节点
			if (typeRet.first == blockcode){
				//判断是代码块起始还是结束
				if (!inBlockcode){
					//起始，创建代码块节点
					_root->_child.push_back(new Node(blockcode));
				}

				inBlockcode = !inBlockcode;
				continue;
			}

			//如果是代码块中的代码
			if (inBlockcode){
				_root->_child.back()->_elem[0] += row;
				_root->_child.back()->_elem[0] += '\n';
				continue;
			}

			//段落节点
			if (typeRet.first == paragraph){
				//创建段落节点
				_root->_child.push_back(new Node(paragraph));
				//逐字符插入
				insert(_root->_child.back(), typeRet.second);

			}

			//标题节点
			if (typeRet.first >= h1 && typeRet.first <= h6){
				//创建标题节点
				_root->_child.push_back(new Node(typeRet.first));
				//插入标题内容
				_root->_child.back()->_elem[0] = typeRet.second;
				continue;
			}

			//无序列表
			if (typeRet.first == ul){
				//判断是否为无序列表的第一项
				//文档开始或者当前语法树的最后一个节点不是无序列表节点
				if (_root->_child.empty() || _root->_child.back()->_type != ul){
					//创建节点
					_root->_child.push_back(new Node(ul));
				}
				//给无序列表加入列表子节点
				Node* unode = _root->_child.back();
				unode->_child.push_back(new Node(li));
				//给列表子节点插入内容
				insert(unode->_child.back(), typeRet.second);
				continue;
			}

			//有序列表
			if (typeRet.first == ol){
				if (_root->_child.empty() || _root->_child.back()->_type != ol){
					_root->_child.push_back(new Node(ol));
				}
				Node* node = _root->_child.back();
				node->_child.push_back(new Node(li));
				insert(node->_child.back(), typeRet.second);
			}

			//引用
			if (typeRet.first == quote){
				//创建引用节点
				_root->_child.push_back(new Node(quote));
				insert(_root->_child.back(), typeRet.second);
			}


		}

		//展开语法树，生成html文档
		dfs(_root);
	}

	//深度优先
	void dfs(Node* root)	{

		//插入前置标签
		_content += frontTag[root->_type];
		//插入节点内容
		//图片和链接是特殊
		//链接
		if (root->_type == href){
			_content += "<a href=\"";
			_content += root->_elem[1];
			_content += "\">";
			_content += root->_elem[0];
			_content += "</a>";
		}
		//图片
		else if (root->_type == image){
			_content += "<img alt-\"";
			_content += root->_elem[0];
			_content += "\" src=\"";
			_content += root->_elem[1];
			_content += "\" />";
		}
		else{
			_content += root->_elem[0];
		}
		//处理孩子节点
		for (Node* child : root->_child)
			dfs(child);

		//插入后置标签
		_content += backTag[root->_type];
	}

	//逐字符插入
	void insert(Node* cur, const char* str){
		//是否为行内代码
		bool incode = false;
		//是否粗体
		bool instrong = false;
		//是否斜体
		bool inem = false;


		//如果是纯文本节点，就创建一个纯文本节点
		cur->_child.push_back(new Node(nul));
		for (int i = 0; i < strlen(str); i++){
			//行内代码
			if (str[i] == '`'){
				if (incode){
					//创建新节点存放后序的内容
					cur->_child.push_back(new Node(nul));
				}
				else{
					cur->_child.push_back(new Node(code));
					
				}
				incode = !incode;
				continue;
			}

			//粗体
			if (str[i] == '*'&&i + 1 < strlen(str) && str[i + 1] == '*'&&!incode){
				if (instrong){
					cur->_child.push_back(new Node(nul));
				}
				else{
					cur->_child.push_back(new Node(strong));
				}
				instrong = !instrong;
				++i;
				continue;
			}

			//斜体
			if (str[i] == '_'&&!incode){
				if (inem){
					cur->_child.push_back(new Node(nul));
				}
				else{
					cur->_child.push_back(new Node(em));
				}
				inem = !inem;
				continue;
			}

			//图片
			if (str[i] == '!'&&i + 1 < strlen(str) && str[i + 1] == '['){
				//创建图片节点
				cur->_child.push_back(new Node(image));
				Node* inode = cur->_child.back();
				i += 2;
				//图片名称
				for (; i < strlen(str) && str[i] != ']'; i++){
					inode->_elem[0] += str[i];
				}
				//图片地址
				i += 2;
				for (; i < strlen(str) && str[i] != '}'; i++){
					inode->_elem[1] += str[i];

				}
				//创建一个新的节点，存放后续内容
				cur->_child.push_back(new Node(nul));
				continue;
			}

			//链接
			if (str[i] == '['&&!incode){
				//创建连接节点
				cur->_child.push_back(new Node(href));
				Node* hnode = cur->_child.back();
				++i;
				for (; i < strlen(str); i++){
					hnode->_elem[0] += str[i];
				}
				i += 2;
				for (; i < strlen(str); i++){
					hnode->_elem[1] += str[i];
				}
				cur->_child.push_back(new Node(nul));
				continue;
			}
			//普通纯文本
			cur->_child.back()->_elem[0] += str[i];
		}

		
	}

	pair<int, const char*>praseType(const char* str){
		//解析标题
		const char* ptr = str;
		int titlenum = 0;
		while (*ptr && *ptr == '#'){
			++ptr;
			++titlenum;
		}
		if (*ptr == ' '&&titlenum>0&&titlenum>=0){
			return make_pair(h1 + titlenum - 1, ptr + 1);
		}

		//不是标题
		ptr = str;

		//代码块
		if (strncmp(ptr, "```", 3) == 0){
			return make_pair(blockcode, ptr + 3);
		}
		//无序列表
		if (strncmp(ptr, "- ", 2) == 0){
			return make_pair(ul, ptr + 2);
		}
		//有序列表
		if (*ptr >= '0'&&*ptr <= '9'){
			//遍历完数字
			while (*ptr && (*ptr >= '0'&&*ptr <= '9'))
				++ptr;
			if (*ptr&&*ptr == '.'){
				++ptr;
				if (*ptr&&*ptr == ' ')
					return make_pair(ol,ptr + 1);
			}
		}
		//重置
		ptr = str;
		//引用
		if (strncmp(ptr, "> ", 2) == 0){
			return make_pair(quote, ptr + 2);
		}

		//其他语法解析为段落
		return make_pair(paragraph, str);
	}
	//水平分割线
	bool isCutLine(const char* str){
		int count = 0;
		while (*str && *str == '-'){
			++str;
			++count;
		}
		return count >= 3;
	}
	//去行首空格
	const char*processStr(const char* str){
		while (str){
			if (*str == ' ' || *str == '\t'){
				str++;
			}
			else{
				break;
			}
			if (*str == '\0')
				return nullptr;
			return str;
		}
	}
	void getHtml(){
		std::string head =
			"<!DOCTYPE html><html><head>\
			<meta charset=\"utf-8\">\
		    <title>Markdown</title>\
			<link rel=\"stylesheet\" href=\"github-markdown.css\">\
			</head><body><article class=\"markdown-body\">";

		std::string end = "</article></body></html>";

		ofstream fout("out/markdown.html");
		fout << head << _content << end;
		cout << "已生成文件" << endl;
	}
	void destory(Node* root)
	{
		if (root)
		{
			for (auto ch : root->_child)
				destory(ch);

			delete root;
		}
	}

	//析构
	~Markdownparser()
	{
		if (_root)
			destory(_root);
	}
private:
	//语法树的根节点
	Node* _root;
	//文件名
	string _filename;
	//存放html文档内容 
	string _content;


};

