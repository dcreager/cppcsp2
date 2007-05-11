/*
*	The Kent C++CSP Library 
*	Copyright (C) 2002-2007 Neil Brown
*
*	This library is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	This library is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with this library; if not, write to the Free Software
*	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "test.h"
#include "../src/cppcsp.h"
#include <iostream>

using namespace std;
using namespace csp;

#ifndef CPPCSP_WINDOWS

#define DEFAULT_COLOUR "\x1B[0m"
#define RED_COLOUR "\x1B[31m"
#define GREEN_COLOUR "\x1B[32m"
#define BRIGHT_WHITE_COLOUR "\x1B[1;37;40m"

inline void printHeader()
{
}

inline void printPassed(std::string s)
{
        std::cout << s << " --> " << GREEN_COLOUR << "Passed" << DEFAULT_COLOUR << " <--" << std::endl;
}

inline void printFailed(std::string s,std::string why)
{
        std::cout << s << " --> " << RED_COLOUR << "Failed" << DEFAULT_COLOUR << " <-- " << why << std::endl;
}

inline void printSummary(int passed,int failed)
{
	cout << endl << "Total tests: " << BRIGHT_WHITE_COLOUR << (passed + failed) << DEFAULT_COLOUR;
	cout << " passed: " << GREEN_COLOUR << passed << DEFAULT_COLOUR;
	cout << " failed: " << RED_COLOUR << failed << DEFAULT_COLOUR << endl;
	
	if (failed == 0)
	{
		cout << GREEN_COLOUR << "************************* All Tests Passed *************************" << DEFAULT_COLOUR << endl;
	}
	else
	{
		cout << RED_COLOUR << "!!!!!!!!!!!!!!!!!!!!!!!!! Some Tests Failed !!!!!!!!!!!!!!!!!!!!!!!!!" << DEFAULT_COLOUR << endl;	
	}
}

#else

#include "tchar.h"

HWND gMainWindow;
extern HINSTANCE ghInstance;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_PAINT:
			if (GetUpdateRect(hWnd,NULL,FALSE) != 0)
			{
				BeginPaint(hWnd,NULL);
				EndPaint(hWnd,NULL);
			}
			break;
		case WM_CLOSE:
			PostQuitMessage(0);
			break;
	}
	
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

using namespace std;

#include <fstream>

ofstream outFile;

inline void printHeader()
{
	//TODO later add datestamp and machine info
	outFile.open("test-results.html");

	outFile << "<html><head><title>C++CSP2 Test Results</title><style type=\"text/css\">";
	outFile << " body {color: #C0C0C0;background-color:#000000;} ";
	outFile << " td.passed {color: #20C020;} ";
	outFile << " .failed {color: #C02020;} ";
	outFile << "</style></head><body><h1><center>Results</center></h1>" << endl;
	
	outFile << "<br/><table><tr><th>Test Name</th><th>Result</th></tr>" << endl;
/*
	WNDCLASS wcls;
	ZeroMemory(&wcls,sizeof(WNDCLASS));
	wcls.lpfnWndProc = WindowProc;
	wcls.hInstance = ghInstance;	
	wcls.lpszClassName = _TEXT("CPPCSP_TestOutput");
	
	RegisterClass(&wcls);
	
	
	gMainWindow = CreateWindow(
		_TEXT("CPPCSP_TestOutput"),
		_TEXT("C++CSP2 Test Results"),
		WS_TILEDWINDOW|WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		ghInstance,
		NULL);
*/	
}

void replaceAll(string* s, string replace,string with)
{
	string::size_type n = 0;
	n = s->find(replace, n);
    while (n != string::npos)
    {
		s->replace(n, replace.size(), with);
        n = s->find(replace, n + with.length());
    }
}

inline void printPassed(std::string s)
{	
	replaceAll(&s,"\n","<br>");

	outFile << "<tr><td>" << s << "</td><td class=\"passed\">Passed</td></tr>" << endl;
}

inline void printFailed(std::string s,std::string why)
{
	replaceAll(&s,"\n","<br>");
	replaceAll(&why,"\n","<br>");

	outFile << "<tr><td>" << s << "</td><td><div class=\"failed\">Failed:</div> " << why << "</td></tr>" << endl;
}

inline void printSummary(int passed,int failed)
{	
	outFile << "</table><br/><center><h1>Passed: " << passed << ", Failed: " << failed << "</h1></center></body></html>" << endl;

	outFile.close();
	
	system("start test-results.html");

	/*
	MSG msg;
	while (GetMessage(&msg,NULL,0,0))
	{		
		DispatchMessage(&msg);		
	}	
	*/
}

#endif


bool runTests(bool incPerfTests)
{	
	list<Test*> allTests = GetAllTests();	

	Start_CPPCSP();
	
	printHeader();
	
	int passed = 0,failed = 0;
	
	for (list<Test*>::iterator it = allTests.begin();it != allTests.end();it++)
	{
		list<TestResult (*)() > tests;

		tests = (*it)->tests();
		if (incPerfTests)
		{
			list<TestResult (*)() > perfTests = (*it)->perfTests();
			tests.splice(tests.end(),perfTests);			
		}
	
		for (list<TestResult (*)()>::iterator jt = tests.begin();jt != tests.end();jt++)
		{
			TestResult result = (*jt)();
		
			if (result.passed())
			{
				printPassed(result.getName());
				passed++;
			}
			else
			{
				printFailed(result.getName(),result.getMessage());
				failed++;
			}
		}
	
		delete *it;
	}
	
	printSummary(passed,failed);
	
	End_CPPCSP();
	
	return (failed == 0);
}
