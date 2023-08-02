
#include <iostream>
#include <cmath>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>



using namespace std;
using namespace cv;

int main3() {
	int file_size = 100; // Snake Case (lower case, multiple words separate)
	int FileSize = 100; // Pascal Case (Upper case, multiple words stick together)
	int fileSize = 100; // Camel Case (first lower case of pascal)
	int iFileSize = 100; // Hungarian Notation (old, editors didn't use to show type, not needed anymore)
	const double pi = 3.14; // const: constant so non changing, like a tuple in python

	int x = 10;
	// increment operator ++, --> + 1 to things applied. decrement --> x--. Two ways of use:
	int y = x++; // y will be set to 10, THEN x = x + 1
	y = ++x; // value of x will be incremented by 1, THEN y = x = 11



	std::cout << "x = " << x << endl; // chaining stream insertion operators

	cout << "x = " << x << endl << "y = " << y << endl;  // compared to the top, std:: is not needed as "using namespace std;" is up there already

	cout << "Enter values for a and b: ";
	double a; // >> stream extraction operator. data goes from cin (console) to variable.
	double b;
	cin >> a >> b; // stream extraction operator can also be chained like stream insertion operators
	cout << a + b << endl;
	
	cout << "Temperature in fahrenheit: ";
	double degreeF;
	cin >> degreeF;
	double degreeC = (degreeF - 32) * 5 / 9;

	cout << "Temperature in Celsius: " << degreeC << endl;

	double result = pow(2, 3); // using cmath from the standard library
	cout << result << endl;
	
	double radius;
	cout << "Radius = ";
	cin >> radius;
	double area = pi * pow(radius, 2);
	cout << "Area = " << area;
	

	/*
	
	multi line comment
	
	*/

	// more data types
	double price = 99.99;
	float interestRate = 3.67f; // float and 
	// long fileSize = 90000L;
	char letter = 'a';   // hehe
	auto letterSecond = 'b';   // auto can detect types
	auto isValid = false;

	int number1{30};  // use braces, empty braces give 0

	int number = 0xFF; // 0x prefix gives a hexadecimal base 16 number, 0b is binary




	return 0;
}

